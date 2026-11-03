// SPDX-License-Identifier: GPL-3.0-or-later
// Apple renderer for Eden Duo's original, data-driven ACNH companion interface.
// The manifest is vendored with its provenance; images and fonts come from the user's RomFS.
import SwiftUI
import Combine

indirect enum EdenExpression {
    case number(Double), key(String), unary(String, EdenExpression)
    case binary(String, EdenExpression, EdenExpression), choose(EdenExpression, EdenExpression, EdenExpression)
    case call(String, [EdenExpression])
    func evaluate(_ get: (String) -> Double) -> Double {
        switch self {
        case .number(let n): return n
        case .key(let k): return get(k)
        case .unary(let op, let a): let x=a.evaluate(get);return op=="!" ? (x==0 ? 1:0) : op=="-" ? -x:x
        case .choose(let c,let a,let b):return (c.evaluate(get) != 0 ? a:b).evaluate(get)
        case .binary(let op,let a,let b):
            let x=a.evaluate(get)
            if op=="&&" && x==0{return 0};if op=="||" && x != 0{return 1}
            let y=b.evaluate(get)
            switch op {
            case "+":return x+y;case "-":return x-y;case "*":return x*y
            case "/":return y==0 ? 0:x/y;case "%":return y==0 ? 0:x.truncatingRemainder(dividingBy:y)
            case "==":return x==y ? 1:0;case "!=":return x != y ? 1:0
            case "<":return x<y ? 1:0;case ">":return x>y ? 1:0;case "<=":return x<=y ? 1:0;case ">=":return x>=y ? 1:0
            case "&&":return y != 0 ? 1:0;case "||":return y != 0 ? 1:0;default:return 0
            }
        case .call(let name,let args):
            let a=args.map{$0.evaluate(get)}
            switch name {case "min":return a.min() ?? 0;case "max":return a.max() ?? 0;case "abs":return abs(a.first ?? 0)
            case "floor":return floor(a.first ?? 0);case "round":return (a.first ?? 0).rounded()
            case "clamp":return a.count==3 ? min(max(a[0],a[1]),a[2]):0;default:return 0}
        }
    }
    static func parse(_ text: String) -> EdenExpression { var p=EdenParser(text);return p.expression() }
}
private struct EdenParser {
    var tokens:[String]=[],index=0
    init(_ source:String) {
        let a=Array(source);var i=0
        while i<a.count {
            let c=a[i];if c.isWhitespace{i+=1;continue}
            if c=="'" || c=="\"" {let q=c;i+=1;let start=i;while i<a.count && a[i] != q{i+=1};tokens.append("'"+String(a[start..<i]));if i<a.count{i+=1};continue}
            if c.isNumber || (c=="." && i+1<a.count && a[i+1].isNumber) {
                let start=i;i+=1;while i<a.count && (a[i].isNumber || a[i]=="."){i+=1};tokens.append(String(a[start..<i]));continue
            }
            if c.isLetter || c=="_" || c=="@" {
                let start=i;i+=1;while i<a.count && (a[i].isLetter || a[i].isNumber || a[i]=="_" || a[i]=="."){i+=1};tokens.append(String(a[start..<i]));continue
            }
            if i+1<a.count {let op=String(a[i...i+1]);if ["&&","||","==","!=","<=",">="].contains(op){tokens.append(op);i+=2;continue}}
            tokens.append(String(c));i+=1
        }
    }
    var peek:String {index<tokens.count ? tokens[index]:""}
    mutating func take()->String {let t=peek;if index<tokens.count{index+=1};return t}
    func priority(_ op:String)->Int {switch op {case "||":return 1;case "&&":return 2;case "==","!=":return 3;case "<",">","<=",">=":return 4;case "+","-":return 5;case "*","/","%":return 6;default:return -1}}
    mutating func expression(_ minimum:Int=0)->EdenExpression {
        let t=take();var a:EdenExpression
        if ["!","-","+"].contains(t){a = .unary(t,expression(7))}
        else if t=="(" {a=expression();if peek==")"{_ = take()}}
        else if let n=Double(t){a = .number(n)}
        else if peek=="(" { _ = take();var args:[EdenExpression]=[];while !peek.isEmpty && peek != ")" {args.append(expression());if peek != ","{break};_ = take()};if peek==")"{_ = take()};a = .call(t,args)}
        else {a = .key(t.hasPrefix("'") ? String(t.dropFirst()):t)}
        while !peek.isEmpty && priority(peek)>=minimum {let op=take(),p=priority(op);a = .binary(op,a,expression(p+1))}
        if minimum==0 && peek=="?" {_ = take();let yes=expression();if peek==":"{_ = take()};a = .choose(a,yes,expression())}
        return a
    }
}
struct EdenWidgetTemplate {
    let index:Int, limit:Int, repeatBinding:String?, instances:[[String:Any]]
    init(_ index:Int,_ raw:[String:Any]) {
        self.index=index
        limit=min(200,max(1,(raw["repeat"] as? NSNumber)?.intValue ?? 1))
        repeatBinding=raw["repeat_bind"] as? String
        instances=(0..<limit).map{EdenFrame.expand(raw,$0) as! [String:Any]}
    }
}
final class EdenManifest {
    static let shared=EdenManifest()
    let root:[String:Any]
    let pages:[String:[String:Any]]
    let derived:[String:[String:Any]]
    let expressions:[String:EdenExpression]
    let widgets:[String:[EdenWidgetTemplate]]
    init(data:Data?=nil) {
        let bytes=data ?? Bundle.main.url(forResource:"companion-ui",withExtension:"json",subdirectory:"dm").flatMap{try? Data(contentsOf:$0)}
        root=(bytes.flatMap{try? JSONSerialization.jsonObject(with:$0)} as? [String:Any]) ?? [:]
        pages=Dictionary(uniqueKeysWithValues:((root["pages"] as? [[String:Any]]) ?? []).compactMap{d in (d["id"] as? String).map{($0,d)}})
        // Only immutable manifest templates are expanded here. Live visibility,
        // geometry, counts and input authorization are still evaluated per frame.
        widgets=pages.mapValues { page in
            ((page["widgets"] as? [[String:Any]]) ?? []).enumerated().compactMap { index,raw in
                let rect=EdenFrame.rectangle(raw["rect"])
                // Upstream's one-pixel waiting art prewarm is intentionally omitted.
                return rect.width==1 && rect.height==1 ? nil:EdenWidgetTemplate(index,raw)
            }
        }
        let entries=(root["derived"] as? [[String:Any]]) ?? []
        derived=Dictionary(uniqueKeysWithValues:entries.compactMap{d in (d["name"] as? String).map{($0,d)}})
        expressions=Dictionary(uniqueKeysWithValues:entries.compactMap{d in guard let n=d["name"] as? String,let e=d["expr"] as? String else{return nil};return(n,EdenExpression.parse(e))})
    }
}
final class EdenBindings {
    let values:[String:Any], manifest:EdenManifest
    var local:[String:Double],cache:[String:Double]=[:],visiting=Set<String>()
    init(_ values:[String:Any],_ local:[String:Double],_ manifest:EdenManifest) {self.values=values;self.local=local;self.manifest=manifest}
    func value(_ any:Any?) -> Double {if let n=any as? NSNumber{return n.doubleValue};if let s=any as? String{return Double(s) ?? number(s)};return 0}
    func number(_ key:String)->Double {
        if key.hasPrefix("!"){return number(String(key.dropFirst()))==0 ? 1:0}
        if let v=local[key]{return v};if let v=cache[key]{return v}
        if let v=values[key] as? NSNumber{return v.doubleValue}
        guard let d=manifest.derived[key],!visiting.contains(key) else {return key.hasPrefix("@sel:") || key.hasPrefix("@map_sel:") ? -1:0}
        visiting.insert(key);defer{visiting.remove(key)};var n=0.0
        if let e=manifest.expressions[key]{n=e.evaluate(number)}
        else if let a=d["all_nonzero"] as? [String]{n=a.allSatisfy{number($0) != 0} ? 1:0}
        else if let a=d["any_nonzero"] as? [String]{n=a.contains{number($0) != 0} ? 1:0}
        else if let op=d["cmp"] as? String {
            let a=value(d["a"]),b=value(d["b"])
            switch op {case "eq":n=a==b ? 1:0;case "ne":n=a != b ? 1:0;case "lt":n=a<b ? 1:0;case "le":n=a<=b ? 1:0;case "gt":n=a>b ? 1:0;case "ge":n=a>=b ? 1:0;default:break}
        } else if let k=d["select"] as? String {n=value(d[number(k) != 0 ? "then":"else"])}
        else if let terms=d["terms"] as? [Any] {n=value(d["add"]);for term in terms {if let t=term as? [Any],t.count==2 {n+=value(t[0])*value(t[1])}else{n+=value(term)}};if d["floor"] as? Bool==true{n=floor(n)};if d["round"] as? Bool==true{n=n.rounded()}}
        if !n.isFinite{n=0};cache[key]=n;return n
    }
    func text(_ key:String)->String {values[key] as? String ?? ""}
    func shown(_ d:[String:Any])->Bool {
        if let a=d["anim"] as? [String:Any],let k=a["bind"] as? String,number(k)==0{return false}
        if let k=d["need_bind"] as? String,number(k)==0{return false}
        if let k=d["hide_bind"] as? String {
            let n=number(k)
            if let eq=d["hide_eq"],n==value(eq){return false}
            if let lo=d["keep_min"],n<value(lo){return false};if let hi=d["keep_max"],n>value(hi){return false}
        }
        return true
    }
}
struct EdenNode:Identifiable {
    let id:Int, data:[String:Any], rect:CGRect, clip:CGRect?
    var kind:String {data["type"] as? String ?? "rect"}
    var action:String {data["on_tap"] as? String ?? ""}
    var payload:Int {Int((data["payload"] as? String).flatMap(Double.init) ?? (data["payload"] as? NSNumber)?.doubleValue ?? 0)}
    var draggable:Bool {data["draggable"] as? Bool==true}
    func contains(_ p:CGPoint)->Bool {rect.contains(p) && (clip?.contains(p) ?? true)}
}
struct EdenFrame {
    let bindings:EdenBindings,page:String,nodes:[EdenNode],scrolls:[[String:Any]]
    static func substitution(_ s:String,_ i:Int)->String {
        var r=s.replacingOccurrences(of:"{i}",with:String(i))
        while let a=r.range(of:"{i+"),let z=r[a.upperBound...].firstIndex(of:"}"),let n=Int(r[a.upperBound..<z]) {r.replaceSubrange(a.lowerBound...z,with:String(i+n))}
        return r
    }
    static func expand(_ value:Any,_ i:Int)->Any {
        if let s=value as? String{return substitution(s,i)}
        if let a=value as? [Any]{return a.map{expand($0,i)}}
        if let d=value as? [String:Any]{return d.mapValues{expand($0,i)}};return value
    }
    static func rectangle(_ any:Any?)->CGRect {guard let a=any as? [NSNumber],a.count==4 else{return .zero};return CGRect(x:a[0].doubleValue,y:a[1].doubleValue,width:a[2].doubleValue,height:a[3].doubleValue)}
    @MainActor init(model:CompanionModel,manifest:EdenManifest,local:[String:Double]) {
        var bindingsLocal=local;bindingsLocal["ui.page"]=Double(model.page.rawValue)
        let b=EdenBindings(model.values,bindingsLocal.merging(["module_ready":1,"acnh.font":1]){a,_ in a},manifest);bindings=b
        var pg=b.number("ui.pg");if pg==0 && model.ready{pg=Double(model.page.rawValue)}
        page = b.number("host.unsupported") != 0 ? "version" : b.number("module_error") != 0 ? "error" : b.number("scene.title") != 0 ? "title" : !model.ready ? "waiting" : [1:"map",2:"bag",3:"critters",4:"diy",5:"today",6:"phone"][Int(pg)] ?? "map"
        let p=manifest.pages[page] ?? [:]
        scrolls=(p["scrolls"] as? [[String:Any]] ?? []).filter { spec in
            b.shown(spec) && ((spec["show_bind"] as? String).map { b.number($0) != 0 } ?? true)
        }
        var result:[EdenNode]=[]
        for template in manifest.widgets[page] ?? [] {
            let index=template.index,limit=template.limit
            let count=template.repeatBinding.map{min(limit,max(0,Int(b.number($0))))} ?? limit
            for i in 0..<count {
                let d=template.instances[i];if !b.shown(d){continue}
                // Our checked native command menu replaces the upstream informational blocker.
                if page=="bag" && model.number("bag.native.open") != 0 && (d["input_block"] as? Bool==true || (d["need_bind"] as? String)?.contains("ui.bag.card")==true){continue}
                var rect=Self.rectangle(d["rect"]),clip:CGRect?
                let cols=max(1,(d["repeat_cols"] as? NSNumber)?.intValue ?? limit)
                rect.origin.x+=Double(i%cols)*b.value(d["repeat_dx"]);rect.origin.y+=Double(i/cols)*b.value(d["repeat_row_dy"])
                if let k=d["x_bind"] as? String{rect.origin.x+=b.number(k)*((d["x_scale"] as? NSNumber)?.doubleValue ?? 1)}
                if let k=d["y_bind"] as? String{rect.origin.y+=b.number(k)*((d["y_scale"] as? NSNumber)?.doubleValue ?? 1)}
                if let s=d["scroll"] as? String {
                    guard let scroll=scrolls.first(where:{$0["id"] as? String==s}) else{continue}
                    if let k=scroll["show_bind"] as? String,b.number(k)==0{continue}
                    clip=Self.rectangle(scroll["rect"]);rect.origin.y-=local["@scroll:"+s] ?? 0
                    if let clip,rect.height>0 && !clip.intersects(rect){continue}
                }
                result.append(EdenNode(id:index*256+i,data:d,rect:rect,clip:clip))
            }
        }
        // Scroll adornments belong directly after their list, before later cards
        // and modal blockers. A separate final paint pass leaked them over sheets.
        let widgets=p["widgets"] as? [[String:Any]] ?? []
        for spec in scrolls {
            guard let id=spec["id"] as? String,
                  let last=widgets.lastIndex(where: { $0["scroll"] as? String == id }) else { continue }
            let r=Self.rectangle(spec["rect"]),cols=max(1,b.value(spec["cols"]))
            let total=ceil(b.number(spec["count_bind"] as? String ?? "")/cols)*b.value(spec["row_h"])
            let width=min(r.width,max(0,b.value(spec["bar_w"])))
            guard r.height>0,width>0,total>r.height,total.isFinite else { continue }
            let height=min(r.height,max(30,r.height*r.height/total))
            let fraction=min(1,max(0,(local["@scroll:"+id] ?? 0)/(total-r.height)))
            let track=CGRect(x:r.maxX-width,y:r.minY,width:width,height:r.height)
            let thumb=CGRect(x:track.minX,y:r.minY+fraction*(r.height-height),width:width,height:height)
            for (offset,rect,color) in [(254,track,spec["bar_track"]),(255,thumb,spec["bar"])] {
                result.append(EdenNode(id:last*256+offset,data:["type":"rect","bg":color ?? "#00000000","pill":true,"apple_scrollbar":id],rect:rect,clip:r))
            }
        }
        nodes=result.sorted { $0.id < $1.id }
    }
    func source(_ node:EdenNode)->String {
        let d=node.data
        if let key=d["src_bind"] as? String{return bindings.text(key)}
        if let names=d["src_names"] as? [String],let bind=d["bind"] as? String,!names.isEmpty{return names[min(names.count-1,max(0,Int(bindings.number(bind))))]}
        return d["src"] as? String ?? ""
    }
    func text(_ node:EdenNode)->String {
        let d=node.data
        if node.kind=="value" {var s=String(Int(bindings.number(d["bind"] as? String ?? "")));if let max=d["max_bind"] as? String{s+=(d["max_sep"] as? String ?? "/")+String(Int(bindings.number(max)))};return ((d["text"] as? String ?? "")+s).replacingOccurrences(of:"\\{/?c(?::[^}]+)?\\}",with:"",options:.regularExpression)}
        if let k=d["bind_text"] as? String{return bindings.text(k)}
        if let map=d["text_map"] as? [String:String],let k=d["text_bind"] as? String {return flavor(map[String(Int(bindings.number(k)))] ?? "")}
        return flavor(d["text_src"] as? String ?? d["text"] as? String ?? "")
    }
    func flavor(_ s:String)->String {
        guard s.hasPrefix("msbt:") else{return s}
        let key=String(s.split(separator:"#").last ?? "")
        let language=bindings.text("acnh.lang")
        let folders=bindings.manifest.root["apple_flavor"] as? [String:[String:String]] ?? [:]
        return folders[language]?[key] ?? folders["USen"]?[key] ?? companionText("Continue on the game screen above.")
    }
    func hit(_ point:CGPoint)->EdenNode? {nodes.reversed().first{$0.contains(point) && (!$0.action.isEmpty || $0.draggable || $0.kind=="map" || $0.data["input_block"] as? Bool==true)}}
}

// Body and navigation have independent transforms so navigation always stays at the bottom.
struct EdenSurfaceLayout {
    let body:CGRect,footer:CGRect
    let bodyScale:Double,navScale:Double
    let bodyOffset:CGPoint,navOffset:CGPoint
    init(_ size:CGSize) {
        let h=dm_companion_footer_height(size),navWidth=max(1,size.width)
        body=CGRect(x:0,y:0,width:size.width,height:max(1,size.height-h))
        footer=CGRect(x:0,y:body.maxY,width:navWidth,height:h)
        bodyScale=max(0.001,min(size.width/1240,body.height/968))
        bodyOffset=CGPoint(x:(size.width-1240*bodyScale)/2,y:(body.height-968*bodyScale)/2)
        navScale=max(0.001,min(navWidth/1240,h/112))
        navOffset=CGPoint(x:(navWidth-1240*navScale)/2,y:body.maxY+(h-112*navScale)/2-968*navScale)
    }
    func point(_ p:CGPoint)->CGPoint {
        let nav=p.y>=footer.minY,s=nav ? navScale:bodyScale,o=nav ? navOffset:bodyOffset
        return CGPoint(x:(p.x-o.x)/s,y:(p.y-o.y)/s)
    }
}

@MainActor private final class EdenImages:ObservableObject {
    @Published var epoch=0
    private let cache=NSCache<NSString,CGImage>()
    private var visible:[String:CGImage]=[:],visibleBytes=0
    private var pending=Set<String>(),failed:[String:Date]=[:]
    private var attempts:[String:Int]=[:],requested=Set<String>()
    private var retry:Task<Void,Never>?
    private var retryAt:Date?,retryGeneration=0
    init(){cache.totalCostLimit=96*1024*1024;cache.countLimit=1024}
    deinit { retry?.cancel() }
    func get(_ key:String)->CGImage? {visible[key] ?? cache.object(forKey:key as NSString)}
    private func updateBudget() {
        let remaining=96*1024*1024-visibleBytes
        // A zero NSCache cost limit means unlimited. Visible images are the working
        // set; only the remaining budget belongs to discardable warm-page images.
        cache.totalCostLimit=max(1,remaining)
        if remaining<=0{cache.removeAllObjects()}
    }
    private func pin(_ image:CGImage,_ key:String) {
        if let old=visible[key]{visibleBytes-=old.bytesPerRow*old.height}
        cache.removeObject(forKey:key as NSString)
        visible[key]=image;visibleBytes+=image.bytesPerRow*image.height;updateBudget()
    }
    private func cancelRetry() {retryGeneration+=1;retry?.cancel();retry=nil;retryAt=nil}
    func suspend() {
        requested=[];cancelRetry()
        let parked=visible;visible=[:];visibleBytes=0;updateBudget()
        for (key,image) in parked{cache.setObject(image,forKey:key as NSString,cost:image.bytesPerRow*image.height)}
        failed.removeAll();attempts.removeAll()
    }
    func trim() { cancelRetry();visible.removeAll();visibleBytes=0;cache.removeAllObjects();updateBudget();failed.removeAll();attempts.removeAll();epoch+=1 }
    private func retryMissing(_ model:CompanionModel) {
        let deadlines=requested.compactMap { key -> Date? in
            guard !pending.contains(key) else{return nil}
            return failed[key].map { $0.addingTimeInterval(min(30,pow(2,Double(attempts[key] ?? 1)))) }
        }
        guard model.presented,let deadline=deadlines.min() else{cancelRetry();return}
        if let scheduled=retryAt,scheduled<=deadline{return}
        // A newly missing asset may need an earlier retry than an older failure.
        // Superseded tasks must not clear a newer request's scheduler state.
        cancelRetry();retryAt=deadline
        let generation=retryGeneration
        let delay=max(0.05,deadline.timeIntervalSinceNow)
        retry=Task { @MainActor [weak self,weak model] in
            do { try await Task.sleep(for:.seconds(delay)) } catch {return}
            guard !Task.isCancelled,let self,let model,self.retryGeneration==generation else{return};self.retry=nil;self.retryAt=nil
            if model.presented {self.request(self.requested,model)}
        }
    }
    func request(_ keys:Set<String>,_ model:CompanionModel) {
        guard model.presented else{suspend();return};requested=keys
        // NSCache may discard objects at any time. Keep the current frame's images
        // alive independently; release those pins as soon as their keys leave view.
        let previous=visible;visible=visible.filter{keys.contains($0.key)}
        visibleBytes=visible.values.reduce(0){$0+$1.bytesPerRow*$1.height};updateBudget()
        for (key,image) in previous where !keys.contains(key){cache.setObject(image,forKey:key as NSString,cost:image.bytesPerRow*image.height)}
        for key in keys where visible[key]==nil{if let image=cache.object(forKey:key as NSString){pin(image,key)}}
        failed=failed.filter{keys.contains($0.key)};attempts=attempts.filter{keys.contains($0.key)}
        for key in keys where key.hasPrefix("module:") && get(key)==nil && !pending.contains(key) {
            if let time=failed[key],Date().timeIntervalSince(time)<min(30,pow(2,Double(attempts[key] ?? 1))){continue};pending.insert(key)
            model.load(key){[weak self] data,w,h in
                var image:CGImage?
                if let data,w>0,h>0,w<=4096,h<=4096,data.count==w*h*4,let provider=CGDataProvider(data:data as CFData) {
                    image=CGImage(width:w,height:h,bitsPerComponent:8,bitsPerPixel:32,bytesPerRow:w*4,space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:CGBitmapInfo(rawValue:CGBitmapInfo.byteOrder32Big.rawValue|CGImageAlphaInfo.last.rawValue),provider:provider,decode:nil,shouldInterpolate:true,intent:.defaultIntent)
                }
                let result=image
                Task{@MainActor in
                    guard let self else{return};self.pending.remove(key)
                    guard model.presented,self.requested.contains(key) else{return}
                    if let result{self.pin(result,key);self.failed.removeValue(forKey:key);self.attempts.removeValue(forKey:key);self.epoch+=1}
                    else{self.failed[key]=Date();self.attempts[key]=min(6,(self.attempts[key] ?? 0)+1)}
                    self.retryMissing(model)
                }
            }
        }
        retryMissing(model)
    }
}
private func edenColor(_ value:Any?,_ fallback:String="#00000000")->Color {
    let s=(value as? String ?? fallback).replacingOccurrences(of:"#",with:"")
    let n=UInt64(s,radix:16) ?? 0
    return Color(.sRGB,red:Double((n>>16)&255)/255,green:Double((n>>8)&255)/255,blue:Double(n&255)/255,opacity:s.count==8 ? Double((n>>24)&255)/255:1)
}
private func edenLabelKey(_ text:String,_ scale:Double,_ color:Any?)->String {
    let safe=CharacterSet.alphanumerics.union(CharacterSet(charactersIn:"-_.~"))
    let encoded=text.addingPercentEncoding(withAllowedCharacters:safe) ?? ""
    let c=(color as? String ?? "#FF746144").replacingOccurrences(of:"#",with:"")
    let rgba=c.count==8 ? String(c.dropFirst(2))+String(c.prefix(2)):c+"ff"
    return "module:acnh/ui/label?t=\(encoded)&px=\(Int(scale*5))&c=\(rgba)"
}

@MainActor struct EdenCompanionView:View {
    @ObservedObject var model:CompanionModel
    @StateObject private var images=EdenImages()
    @State private var local:[String:Double]=["@sel:res":-1,"@map_sel:hit":-1,"@flag:theme":Double(UserDefaults.standard.integer(forKey:"DmCompanionTheme"))]
    @State private var dragging:EdenNode?
    @State private var dragPoint=CGPoint.zero
    @State private var dragToken=""
    @State private var startScroll:Double=0
    @State private var mapZoom:Double=1
    @State private var pinchStart:Double=1
    @State private var mapPan=CGSize.zero
    @State private var startPan=CGSize.zero
    @State private var movingSlot:Int?
    @State private var movingToken=""
    @GestureState private var dragTouch=false
    @GestureState private var pinchTouch=false
    @State private var gestureActive=false
    private let manifest:EdenManifest
    init(model:CompanionModel,manifest:EdenManifest = .shared) {_model=ObservedObject(wrappedValue:model);self.manifest=manifest}
    private func labelLines(_ text:String,_ node:EdenNode)->[String] {
        guard let width=node.data["wrap_width"] as? NSNumber,width.doubleValue>0 else{return text.components(separatedBy:"\n")}
        let cap=((node.data["text_scale"] as? NSNumber)?.doubleValue ?? 4)*5
        let chars=max(1,Int(width.doubleValue/cap)),maxLines=(node.data["max_lines"] as? NSNumber)?.intValue ?? 3
        var lines:[String]=[],line="",size=0.0
        for c in text {let advance=c.unicodeScalars.contains{$0.value>=0x2E80} ? 1.0:0.6;if c=="\n" || size+advance>Double(chars){lines.append(line);line="";size=0};if c != "\n"{line.append(c);size+=advance}}
        if !line.isEmpty{lines.append(line)};return Array(lines.prefix(maxLines))
    }
    private func mapNodes(_ n:EdenNode,_ f:EdenFrame)->[EdenNode] {
        let b=f.bindings,area=(manifest.root["map"] as? [String:Any])?["areas"] as? [String:Any]
        guard let spec=area?[n.data["area"] as? String ?? "island"] as? [String:Any] else{return []}
        let minXY=spec["min"] as? [Double] ?? [60,-1175],maxXY=spec["max"] as? [Double] ?? [1300,-106]
        let x0=b.number("ui.mapv.rx0.0"),y0=b.number("ui.mapv.ry0.0"),x1=b.number("ui.mapv.rx1.0"),y1=b.number("ui.mapv.ry1.0")
        let vx=x1>x0 ? x0:minXY[0],vy=y1>y0 ? y0:minXY[1],vw=x1>x0 ? x1-x0:maxXY[0]-minXY[0],vh=y1>y0 ? y1-y0:maxXY[1]-minXY[1]
        let sx=n.rect.width/vw*mapZoom,sy=n.rect.height/vh*mapZoom
        func point(_ x:Double,_ y:Double)->CGPoint {CGPoint(x:n.rect.midX+(x-vx-vw/2)*sx+mapPan.width,y:n.rect.midY-(y-vy-vh/2)*sy+mapPan.height)}
        let p=point(minXY[0],maxXY[1]);var nodes=[EdenNode(id:-1,data:["type":"image","src":b.text(n.data["image_bind"] as? String ?? "map.base.key")],rect:CGRect(x:p.x,y:p.y,width:(maxXY[0]-minXY[0])*sx,height:(maxXY[1]-minXY[1])*sy),clip:n.rect)]
        let groups=n.data["groups"] as? [String:[String:Any]] ?? [:]
        for (index,m) in ((spec["dynamic_markers"] as? [[String:Any]]) ?? []).enumerated() {
            let group=m["group"] as? String ?? ""
            if let g=groups[group] {if let show=g["show_bind"] as? String,b.number(show)==0{continue};if mapZoom<b.value(g["min_zoom"]){continue}}
            let scale=m["scale"] as? [Double] ?? [1,1],anchor=m["anchor"] as? [Double] ?? [0.5,0.5]
            for i in 0..<min(100,(m["count"] as? NSNumber)?.intValue ?? 0) {
                let d=EdenFrame.expand(m,i) as! [String:Any]
                if let k=d["kind"] as? String,let hide=d["hide_when_kind"],b.number(k)==b.value(hide){continue}
                let key=b.text(d["icon_src_bind"] as? String ?? "");if key.isEmpty{continue}
                let p=point(b.number(d["x"] as? String ?? "")*scale[0],b.number(d["y"] as? String ?? "")*scale[1])
                let world=b.value(d["size_world"]),size=b.value(d["size"])
                let w=world>0 ? world*sx:size,h=world>0 ? world*sy:size
                let rect=CGRect(x:p.x-w*anchor[0],y:p.y-h*anchor[1],width:w,height:h)
                if !n.rect.intersects(rect){continue}
                var data:[String:Any]=["type":"image","src":key]
                if group=="hit" {data["on_tap"]="fac_pick";data["payload"]=String(i)}
                nodes.append(EdenNode(id:-100-index*100-i,data:data,rect:rect,clip:n.rect))
            }
        }
        return nodes
    }
    private func clampedPan(_ proposed:CGSize,_ f:EdenFrame)->CGSize {
        guard let map=f.nodes.first(where:{$0.kind=="map"}),
              let areas=(manifest.root["map"] as? [String:Any])?["areas"] as? [String:[String:Any]],
              let area=areas[map.data["area"] as? String ?? ""],let lo=area["min"] as? [Double],let hi=area["max"] as? [Double],lo.count==2,hi.count==2 else{return .zero}
        let b=f.bindings,x0=b.number("ui.mapv.rx0.0"),x1=b.number("ui.mapv.rx1.0"),y0=b.number("ui.mapv.ry0.0"),y1=b.number("ui.mapv.ry1.0")
        guard x1>x0,y1>y0 else{return .zero}
        let sx=map.rect.width/(x1-x0)*mapZoom,sy=map.rect.height/(y1-y0)*mapZoom
        let left=map.rect.width/2+(lo[0]-(x0+x1)/2)*sx,right=map.rect.width/2+(hi[0]-(x0+x1)/2)*sx
        let top=map.rect.height/2-(hi[1]-(y0+y1)/2)*sy,bottom=map.rect.height/2-(lo[1]-(y0+y1)/2)*sy
        return CGSize(width:min(-left,max(map.rect.width-right,proposed.width)),height:min(-top,max(map.rect.height-bottom,proposed.height)))
    }
    private func extraNodes(_ f:EdenFrame)->[EdenNode] {
        guard f.page=="bag",model.number("bag.can_write") != 0 || model.number("bag.can_use") != 0 else{return []};var nodes:[EdenNode]=[]
        func chip(_ id:Int,_ rect:CGRect,_ title:String,_ action:String) {
            nodes.append(EdenNode(id:id,data:["type":"image","src":"module:acnh/ui/capsule?w=\(Int(rect.width))&h=\(Int(rect.height))&c=fff1c3","on_tap":action,"apple_label":companionText(title)],rect:rect,clip:nil))
            nodes.append(EdenNode(id:id+1,data:["type":"label","text":companionText(title),"text_scale":4,"color":"#FF746144","align":"center"],rect:CGRect(x:rect.midX,y:rect.midY-10,width:0,height:0),clip:nil))
        }
        let selected=(model.values["bag.sel"] as? NSNumber)?.intValue ?? -1,busy=model.number("bag.busy") != 0
        if model.number("bag.native.open") != 0 {
            nodes.append(EdenNode(id:900000,data:["type":"rect","bg":"#6504554F","input_block":true],rect:CGRect(x:0,y:0,width:1240,height:964),clip:nil))
            let count=model.count("bag.native.count",max:7),height=Double(count+1)*78+94
            nodes.append(EdenNode(id:900001,data:["type":"image","src":"module:acnh/ui/card?w=700&h=\(Int(height))&r=46&c=fffbe8"],rect:CGRect(x:270,y:140,width:700,height:height),clip:nil))
            let item=model.text("bag.\(model.number("bag.native.slot")).name",companionText("Pockets"))
            nodes.append(EdenNode(id:900002,data:["type":"label","text":item,"text_scale":6,"color":"#FF746144","align":"center"],rect:CGRect(x:620,y:165,width:0,height:0),clip:nil))
            for i in 0..<count {chip(900010+i*2,CGRect(x:305,y:220+Double(i)*78,width:630,height:64),model.text("bag.native.\(i).name"),"apple.command.\(i)")}
            chip(900100,CGRect(x:405,y:220+Double(count)*78,width:430,height:64),"Cancel","apple.close")
        } else {
            if model.number("scene.bag") != 0 && !busy {chip(901050,CGRect(x:808,y:708,width:392,height:64),"Close game pockets","apple.close")}
            if busy {chip(901060,CGRect(x:808,y:875,width:392,height:64),"Cancel","apple.cancel")}
            if selected>=0 && selected<40 && !model.pocketToken(selected).isEmpty && model.number("bag.can_use") != 0 && !busy {chip(901000,CGRect(x:808,y:796,width:392,height:64),"Use / actions","apple.use")}
            if model.number("bag.can_write") != 0 && !busy {
                if selected>=0 && selected<40 && !model.pocketToken(selected).isEmpty {chip(901010,CGRect(x:808,y:875,width:188,height:64),movingSlot==nil ? "Move":"Cancel","apple.move")}
                chip(901020,CGRect(x:1010,y:875,width:190,height:64),"Sort pockets","apple.sort")
            }
            if movingSlot != nil {nodes.append(EdenNode(id:901030,data:["type":"label","text":companionText("Tap a destination slot"),"text_scale":4,"color":"#FF746144","align":"center"],rect:CGRect(x:620,y:947,width:0,height:0),clip:nil))}
        }
        return nodes
    }
    private func renderNodes(_ f:EdenFrame)->[EdenNode] {var a:[EdenNode]=[];for n in f.nodes {if n.kind=="map"{a.append(contentsOf:mapNodes(n,f))};a.append(n)};a+=extraNodes(f);return a}
    private func assetKeys(_ nodes:[EdenNode],_ f:EdenFrame)->Set<String> {
        var keys=Set<String>()
        for n in nodes {if n.kind=="image"{let k=f.source(n);if !k.isEmpty{keys.insert(k)}}
            if n.kind=="label" || n.kind=="value" {let s=(n.data["text_scale"] as? NSNumber)?.doubleValue ?? 4;for line in labelLines(f.text(n),n) where !line.isEmpty{keys.insert(edenLabelKey(line,s,n.data["color"]))}}
        };return keys
    }
    private func draw(_ nodes:[EdenNode],_ f:EdenFrame,_ context:inout GraphicsContext) {
        for n in nodes {
            var c=context;if let clip=n.clip{c.clip(to:Path(clip))};let d=n.data,r=n.rect
            switch n.kind {
            case "image":if let im=images.get(f.source(n)){c.draw(Image(decorative:im,scale:1),in:r)}
            case "label","value":
                let scale=(d["text_scale"] as? NSNumber)?.doubleValue ?? 4,cap=scale*5,align=d["align"] as? String ?? "left"
                for (i,line) in labelLines(f.text(n),n).enumerated() where !line.isEmpty {
                    let y=r.minY+Double(i)*(cap+((d["line_gap"] as? NSNumber)?.doubleValue ?? 4))
                    if let im=images.get(edenLabelKey(line,scale,d["color"])) {
                        let w=Double(im.width),h=Double(im.height),x=r.minX-(align=="center" ? w/2:align=="right" ? w:0)
                        if let outline=d["outline"] {c.addFilter(.shadow(color:edenColor(outline),radius:(d["outline_px"] as? NSNumber)?.doubleValue ?? 1))}
                        c.draw(Image(decorative:im,scale:1),in:CGRect(x:x,y:y,width:w,height:h))
                    } else {
                        let text=Text(line).font(.system(size:cap*1.34,weight:.bold,design:.rounded)).foregroundColor(edenColor(d["color"],"#FF746144"))
                        c.draw(text,at:CGPoint(x:r.minX,y:y),anchor:align=="center" ? .top:align=="right" ? .topTrailing:.topLeading)
                    }
                }
            case "rect","button","bar":
                let radius=d["pill"] as? Bool==true ? min(r.width,r.height)/2:0,path=Path(roundedRect:r,cornerRadius:radius)
                c.fill(path,with:.color(edenColor(d["bg"])))
                if n.kind=="bar" {let total=f.bindings.number(d["max_bind"] as? String ?? ""),amount=f.bindings.number(d["bind"] as? String ?? "");var fill=r;fill.size.width*=total>0 ? min(1,max(0,amount/total)):0;c.fill(Path(fill),with:.color(edenColor(d["color"])))}
                else {let border=(d["border"] as? NSNumber)?.doubleValue ?? (d["frame"] as? NSNumber)?.doubleValue ?? 0;if border>0{c.stroke(path,with:.color(edenColor(d["color"])),lineWidth:border)}}
            default:break
            }
        }
        if let dragging,dragging.draggable,dragPoint != .zero,let im=images.get(f.source(dragging)) {
            var c=context;c.opacity=0.87;c.draw(Image(decorative:im,scale:1),in:CGRect(x:dragPoint.x-dragging.rect.width*0.6,y:dragPoint.y-dragging.rect.height*0.6,width:dragging.rect.width*1.2,height:dragging.rect.height*1.2))
        }
    }
    private func action(_ n:EdenNode,_ f:EdenFrame) {
        if n.data["input_block"] as? Bool==true && n.action.isEmpty{return}
        let name=n.action,payload=n.payload
        if name=="fac_pick"{local["@map_sel:hit"]=Double(payload);mapZoom=1;pinchStart=1;mapPan = .zero}
        if name.hasPrefix("crit_"){local["@scroll:crit"]=0}
        if name.hasPrefix("diy_"){local["@scroll:diy"]=0}
        if name.hasPrefix("apple.command."),let i=Int(name.dropFirst(14)){model.pocketCommand(i);return}
        switch name {
        case "apple.use":model.usePocket(model.number("bag.sel"));return
        case "apple.cancel":model.action("bag_cancel",0);return
        case "apple.close":model.action("bag_close",0);return
        case "apple.sort":movingSlot=nil;model.action("bag_sort",0);return
        case "apple.move":if movingSlot != nil{movingSlot=nil}else{movingSlot=model.number("bag.sel");movingToken=model.pocketToken(movingSlot!)};return
        default:break
        }
        if let from=movingSlot,let id=n.data["id"] as? String,id.hasPrefix("bag_slot_"),let destination=Int(id.dropFirst(9)) {
            _ = model.movePocket(from:from,to:destination,expected:movingToken);movingSlot=nil;return
        }
        if let group=n.data["select_group"] as? String {local["@sel:"+group]=Double(payload)}
        guard let d=(manifest.root["actions"] as? [String:[String:Any]])?[name] else{return}
        if let enabled=d["enabled_bind"] as? String,f.bindings.number(enabled)==0{return}
        let argument=(d["argument"] as? String)=="$payload" ? payload:Int(f.bindings.value(d["argument"]))
        switch d["kind"] as? String {
        case "module":
            let a=d["action"] as? String ?? ""
            if a=="ui_open",let page=CompanionPage(rawValue:argument){movingSlot=nil;model.select(page);local["ui.page"]=Double(argument)}
            else {model.action(a,argument)}
        case "flag":let flag=d["flag"] as? String ?? "";let v=(d["value"] as? String)=="$payload" ? Double(payload):f.bindings.value(d["value"]);local["@flag:"+flag]=v;if flag=="theme"{UserDefaults.standard.set(Int(v),forKey:"DmCompanionTheme")}
        case "map_select":local["@map_sel:"+(d["group"] as? String ?? "hit")]=f.bindings.value(d["value"]);mapZoom=1;pinchStart=1;mapPan = .zero
        case "view_reset":mapZoom=1;pinchStart=1;mapPan = .zero;local["@sel:res"] = -1;local["@map_sel:hit"] = -1
        case "button":model.action("host.press."+(d["button"] as? String ?? "a").lowercased(),120)
        default:break
        }
    }
    private func tap(_ p:CGPoint,_ nodes:[EdenNode],_ f:EdenFrame) {
        // Marker targets are composited inside the map, above its terrain gesture surface.
        if let n=nodes.reversed().first(where:{$0.contains(p) && (!$0.action.isEmpty || $0.data["input_block"] as? Bool==true)}) {action(n,f)}
    }
    private func gestureHit(_ point:CGPoint,_ nodes:[EdenNode],_ f:EdenFrame)->EdenNode? {
        guard let top=nodes.reversed().first(where:{$0.contains(point) && (!$0.action.isEmpty || $0.draggable || $0.kind=="map" || $0.data["input_block"] as? Bool==true)}) else{return nil}
        if let id=top.data["id"] as? String,id.hasPrefix("bag_slot_"),let source=nodes.reversed().first(where:{$0.draggable && $0.data["id"] as? String==id && $0.contains(point)}){return source}
        if top.action=="fac_pick",let map=f.nodes.first(where:{$0.kind=="map" && $0.contains(point)}){return map}
        return top
    }
    private func accessibleNodes(_ nodes:[EdenNode])->[EdenNode] {
        var seen=Set<String>(),result:[EdenNode]=[]
        for n in nodes.reversed() where !n.action.isEmpty {
            if model.number("bag.native.open") != 0 && n.id<900000 && !n.action.hasPrefix("tab."){continue}
            let id=n.data["id"] as? String ?? "\(n.action).\(n.payload).\(n.id)"
            if seen.insert(id).inserted{result.append(n)}
        }
        return result.reversed()
    }
    private func beginInteraction() {
        if !gestureActive {gestureActive=true;model.uiActivity?(1,true)}
    }
    private func endInteraction() {
        if gestureActive {gestureActive=false;model.uiActivity?(1,false)}
    }
    private var localOperation:Bool {movingSlot != nil || (local["@flag:theme_sheet"] ?? 0) != 0}
    var body:some View {
        let f=EdenFrame(model:model,manifest:manifest,local:local),nodes=renderNodes(f)
        let bodyNodes=nodes.filter{$0.rect.minY<960},navNodes=nodes.filter{$0.rect.minY>=960}
        let keys=assetKeys(nodes,f)
        GeometryReader{geometry in
            let layout=EdenSurfaceLayout(geometry.size)
            Canvas{context,_ in
                // Bleed the original background to every edge; content retains its aspect.
                if let background=bodyNodes.first(where:{$0.kind=="image" && $0.rect.width==1240 && $0.rect.height==1080}),let image=images.get(f.source(background)) {
                    context.draw(Image(decorative:image,scale:1),in:CGRect(origin:.zero,size:geometry.size))
                }
                var body=context;body.clip(to:Path(layout.body));body.translateBy(x:layout.bodyOffset.x,y:layout.bodyOffset.y);body.scaleBy(x:layout.bodyScale,y:layout.bodyScale);draw(bodyNodes,f,&body)
                var nav=context;nav.clip(to:Path(layout.footer));nav.translateBy(x:layout.navOffset.x,y:layout.navOffset.y);nav.scaleBy(x:layout.navScale,y:layout.navScale);draw(navNodes,f,&nav)

            }
            .background(edenColor(manifest.root["background"],"#FFFFF1C3"))
            .contentShape(Rectangle())
            .gesture(DragGesture(minimumDistance:0).updating($dragTouch){_,active,_ in active=true}.onChanged{g in
                beginInteraction()
                let p=layout.point(g.location),start=layout.point(g.startLocation)
                if dragging==nil {dragging=gestureHit(start,nodes,f);dragToken=dragging.map{model.pocketToken($0.payload)} ?? "";startPan=mapPan;if let scroll=f.scrolls.first(where:{EdenFrame.rectangle($0["rect"]).contains(start)}){startScroll=local["@scroll:"+(scroll["id"] as? String ?? "")] ?? 0}}
                let distance=hypot(g.translation.width,g.translation.height)/layout.bodyScale
                if distance>12,let dragging,dragging.draggable{dragPoint=p}
                else if distance>12,let scroll=f.scrolls.first(where:{EdenFrame.rectangle($0["rect"]).contains(start)}) {
                    let id=scroll["id"] as? String ?? "",r=EdenFrame.rectangle(scroll["rect"]),total=ceil(f.bindings.number(scroll["count_bind"] as? String ?? "")/max(1,f.bindings.value(scroll["cols"])))*f.bindings.value(scroll["row_h"])
                    local["@scroll:"+id]=min(max(0,total-r.height),max(0,startScroll-(p.y-start.y)))
                } else if distance>12,dragging?.kind=="map"{mapPan=clampedPan(CGSize(width:startPan.width+p.x-start.x,height:startPan.height+p.y-start.y),f)}
            }.onEnded{g in
                let p=layout.point(g.location),distance=hypot(g.translation.width,g.translation.height)/layout.bodyScale
                if distance>12,let source=dragging,source.draggable {
                    if let target=nodes.reversed().first(where:{$0.contains(p) && $0.data["drop_action"] != nil}) {
                        if let name=target.data["drop_action"] as? String,name.hasPrefix("bag.drop."),let slot=Int(name.dropFirst(9)),source.payload<40{_ = model.movePocket(from:source.payload,to:slot,expected:dragToken)}
                        else {var data=target.data;data["on_tap"]=data["drop_action"];data["payload"]=String(source.payload);action(EdenNode(id:target.id,data:data,rect:target.rect,clip:target.clip),f)}
                    }
                } else if distance<=12{tap(p,nodes,f)}
                dragging=nil;dragPoint = .zero;dragToken="";if !pinchTouch{endInteraction()}
            })
            .simultaneousGesture(MagnificationGesture().updating($pinchTouch){_,active,_ in active=true}.onChanged{v in beginInteraction();if f.page=="map" {mapZoom=min(3,max(1,pinchStart*v));mapPan=clampedPan(mapPan,f)}}.onEnded{_ in pinchStart=mapZoom})
            .accessibilityRepresentation {
                ForEach(accessibleNodes(nodes)){n in
                    Button(n.data["apple_label"] as? String ?? accessibilityLabel(n,f)){model.uiActivity?(1,true);action(n,f);model.uiActivity?(1,false)}.accessibilityIdentifier("duo."+(n.data["id"] as? String ?? n.action)+".\(n.payload)")
                }
            }
        }
        .task(id:keys){images.request(keys,model)}
        .onChange(of:dragTouch || pinchTouch){active in if !active{endInteraction()}}
        .onChange(of:localOperation){busy in model.uiActivity?(2,busy)}
        .onChange(of:model.page){_ in movingSlot=nil;dragging=nil;dragPoint = .zero}
        .onChange(of:model.presented){visible in if !visible {images.suspend();movingSlot=nil;dragging=nil;dragPoint = .zero;endInteraction();model.uiActivity?(2,false)}else{images.request(keys,model)}}
        #if os(iOS)
        .onReceive(NotificationCenter.default.publisher(for:UIApplication.didReceiveMemoryWarningNotification)){_ in images.trim();images.request(keys,model)}
        #endif
        .onDisappear{movingSlot=nil;dragging=nil;dragPoint = .zero;endInteraction();model.uiActivity?(2,false);model.action("bag_cancel",0)}
    }
    private func accessibilityLabel(_ n:EdenNode,_ f:EdenFrame)->String {
        if n.action.hasPrefix("tab."),let p=["tab.map":CompanionPage.map,"tab.bag":.bag,"tab.critters":.critters,"tab.diy":.diy,"tab.today":.today,"tab.phone":.phone][n.action]{return p.title}
        if let id=n.data["id"] as? String,id.hasPrefix("bag_slot_"),let slot=Int(id.dropFirst(9)){let title=model.text("bag.\(slot).name");return "\(slot+1): \(title.isEmpty ? companionText("Empty slot"):title)"}
        if n.action=="bag_sel"{return "\(n.payload+1): \(model.text("bag.\(n.payload).name",companionText("Pockets")))"}
        if n.action=="res_sel"{return model.text("res.\(n.payload).name","Resident \(n.payload+1)")}
        return n.action
    }
}
