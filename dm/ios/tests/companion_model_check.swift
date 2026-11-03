import Foundation
@main struct CompanionModelCheck {
    @MainActor static func main() {
        var snapshot: [String: Any] = ["island.name": "Test island", "acnh.ready": 1, "kabu.now": 0, "kabu.0": 123,
            "map.ib.64.0.k0": "module:marker", "map.ib.64.0.x": 1000, "map.ib.64.0.y": 2000,
            "label": "\u{e20e}Rescue", "bag.slots": 100, "crit.n": -1]
        var sent: [(String, Int64)] = []
        let model = CompanionModel(reader: { snapshot }, sender: { sent.append(($0, $1)) }, images: { _, done in done(nil, 0, 0) })
        assert(model.ready && model.islandName == "Test island")
        assert(model.turnipPrice == 123) // Slot zero is Monday AM, not a missing price.
        assert(model.count("bag.slots", max: 40) == 40 && model.count("crit.n") == 0)
        assert(model.text("label") == "Rescue")
        assert(model.markers.count == 1 && model.markers[0].x == 100 && model.markers[0].y == 200 && model.markers[0].box == 64)
        model.select(.phone);assert(sent.last?.0 == "ui_open" && sent.last?.1 == 6)
        snapshot = ["kabu.now": 6, "kabu.6": 321, "nmp.0.miles": 150, "nmp.0.x5": 1, "nmp.1.miles": 100, "nmp.1.card": "module:quest?w=220&bonus=1&pw=110"]
        model.poll();assert(model.turnipPrice == 321 && model.islandName == "Test island")
        assert(model.questMiles(0) == 750 && model.questMiles(1) == 200)
        snapshot = ["kabu.now": -1, "kabu.sun": 97];model.poll();assert(model.turnipPrice == 97)
        snapshot = ["kabu.now": 12, "kabu.12": 999];model.poll();assert(model.turnipPrice == 0)
        snapshot = ["kabu.now": 6, "kabu.6": -1];model.poll();assert(model.turnipPrice == 0)
        snapshot = [:];model.poll();assert(model.turnipPrice == 0 && !model.ready)
        let source = "0102030405060708", target = "feff000000000000"
        snapshot = ["bag.slots": 20, "bag.can_write": 1, "bag.can_use": 1, "bag.busy": 0, "bag.0.token": source, "bag.6.token": target]
        model.poll()
        let before = sent.count
        assert(model.number("bag.can_write") == 0 && model.number("bag.can_use") == 0)
        assert(!model.movePocket(from: 0, to: 6, expected: source))
        model.usePocket(0);model.pocketCommand(1)
        assert(sent.count == before)
        model.select(.bag);model.select(.map);assert(sent.suffix(2).first?.0 == "bag_cancel")
        let recipe: [String: Any] = ["derived": [
            ["name":"simple", "terms":["a","b"], "add":-1],
            ["name":"sum", "terms":[["a",2],["b",3]], "add":1],
            ["name":"choice", "expr":"a * 2 + b == 7 ? ('@sel:res' == -1 ? 8 : 9) : 0"],
            ["name":"gate", "all_nonzero":["sum","choice"]]
        ], "pages":[["id":"map","widgets":[
            ["type":"rect","rect":[0,0,20,30],"need_bind":"gate","on_tap":"hit"],
            ["type":"rect","rect":[0,40,20,30],"hide_bind":"a","hide_eq":2,"on_tap":"hidden"],
            ["type":"image","rect":[10,80,20,20],"repeat":3,"repeat_dx":30,"src_bind":"icon.{i}","payload":"{i}","on_tap":"select"],
            ["type":"rect","rect":[0,0,10,10],"anim":["bind":"missing"],"on_tap":"hidden-animation"]
        ]]]]
        let manifest=EdenManifest(data:try! JSONSerialization.data(withJSONObject:recipe))
        let bindings=EdenBindings(["a":2,"b":3],["@sel:res":-1],manifest)
        assert(bindings.number("simple")==4 && bindings.number("sum")==14 && bindings.number("choice")==8 && bindings.number("gate")==1)
        assert(EdenExpression.parse("0 && 1 / 0 || max(2, 5) >= 5").evaluate({_ in 0})==1)
        assert(!bindings.shown(["need_bind":"!gate"]))
        assert(bindings.shown(["hide_bind":"a"])) // A bare bind does not hide a widget.
        snapshot=["a":2,"b":3,"acnh.ready":1];model.poll();model.page = .map
        let frame=EdenFrame(model:model,manifest:manifest,local:["@sel:res":-1])
        assert(frame.nodes.count==4 && frame.nodes[3].rect.minX==70)
        assert(frame.hit(CGPoint(x:5,y:45))==nil && frame.hit(CGPoint(x:5,y:5))?.action=="hit")
        assert(frame.nodes[3].payload==2 && frame.source(frame.nodes[3]).isEmpty)
        assert(EdenFrame.substitution("slot.{i+2}.{i}",3)=="slot.5.3")
        let changingManifest=EdenManifest(data:try! JSONSerialization.data(withJSONObject:["pages":[["id":"map","widgets":[
            ["type":"image","rect":[0,0,20,20],"repeat":4,"repeat_bind":"total","need_bind":"show.{i}","x_bind":"shift.{i}","repeat_dx":30,"src_bind":"icon.{i+2}","on_tap":"select","payload":"{i}","nested":["label":"row.{i}"]],
            ["type":"image","rect":[0,0,1,1],"repeat":200,"src":"module:prewarm.{i}"]
        ]]]]))
        snapshot=["acnh.ready":1,"total":3,"show.0":1,"show.1":1,"show.2":0,"show.3":1,"shift.1":5,"icon.3":"module:first"]
        model.poll()
        let initial=EdenFrame(model:model,manifest:changingManifest,local:[:])
        assert(initial.nodes.count==2 && initial.nodes[1].rect.minX==35 && initial.nodes[1].payload==1)
        assert(initial.source(initial.nodes[1])=="module:first" && (initial.nodes[1].data["nested"] as? [String:String])?["label"]=="row.1")
        snapshot["total"]=99;snapshot["show.0"]=0;snapshot["show.2"]=1;snapshot["icon.3"]="module:second";model.poll()
        let changed=EdenFrame(model:model,manifest:changingManifest,local:["shift.1":11])
        assert(changed.nodes.map(\.payload)==[1,2,3] && changed.nodes[0].rect.minX==41)
        assert(changed.source(changed.nodes[0])=="module:second" && initial.source(initial.nodes[1])=="module:first")
        assert(changed.hit(CGPoint(x:42,y:5))?.payload==1 && initial.hit(CGPoint(x:1,y:5))?.payload==0)
        snapshot["total"] = -1;model.poll()
        assert(EdenFrame(model:model,manifest:changingManifest,local:[:]).nodes.isEmpty)
        for size in [CGSize(width:834,height:690),CGSize(width:1024,height:750),CGSize(width:500,height:420)] {
            let l=EdenSurfaceLayout(size);assert(l.body.width==size.width && l.footer.maxY==size.height)
            assert(l.footer.width==size.width && !l.body.intersects(l.footer))
            let original=CGPoint(x:600,y:1010),screen=CGPoint(x:l.navOffset.x+original.x*l.navScale,y:l.navOffset.y+original.y*l.navScale)
            let roundTrip=l.point(screen);assert(abs(roundTrip.x-original.x)<0.00001 && abs(roundTrip.y-original.y)<0.00001)
        }
        let scrollManifest=EdenManifest(data:try! JSONSerialization.data(withJSONObject:["pages":[["id":"diy","scrolls":[["id":"diy","rect":[0,0,200,100],"show_bind":"unblocked","count_bind":"count","row_h":80,"cols":2,"bar_w":12,"bar":"#FF112233","bar_track":"#FF445566"]],"widgets":[["type":"rect","rect":[0,0,40,40],"scroll":"diy","on_tap":"recipe"],["type":"rect","rect":[0,0,200,100],"input_block":true,"on_tap":"overlay"]]]]]))
        snapshot=["acnh.ready":1,"count":50,"unblocked":1];model.select(.diy)
        let scrolling=EdenFrame(model:model,manifest:scrollManifest,local:["@scroll:diy":99999])
        let bars=scrolling.nodes.filter{$0.data["apple_scrollbar"] as? String=="diy"}
        assert(bars.count==2 && scrolling.nodes.last?.action=="overlay")
        assert(bars.allSatisfy{$0.rect.maxY<=100 && $0.rect.minY>=0 && $0.clip != nil})
        snapshot["unblocked"]=0;model.poll()
        let covered=EdenFrame(model:model,manifest:scrollManifest,local:[:])
        assert(covered.scrolls.isEmpty && covered.nodes.count==1 && covered.nodes[0].action=="overlay")
        var reads=0
        let idle=CompanionModel(reader:{reads+=1;return ["acnh.ready":1,"island.name":"Unchanged"]},sender:{_,_ in},images:{_,done in done(nil,0,0)})
        var redraws=0;let observation=idle.objectWillChange.sink{redraws+=1}
        for _ in 0..<10{idle.poll()};assert(redraws==0)
        idle.visible(false);let hiddenReads=reads;for _ in 0..<10{idle.poll()};assert(reads==hiddenReads)
        idle.visible(true);assert(reads>hiddenReads);withExtendedLifetime(observation){}
        idle.visible(false);let stoppedReads=reads
        idle.setPresentation(controlsHidden:true,physicalController:false,presented:false)
        assert(reads==stoppedReads && idle.controlsHidden && !idle.presented)
        idle.setPresentation(controlsHidden:false,physicalController:true,presented:true)
        assert(reads==stoppedReads+1 && !idle.controlsHidden && idle.externalController && idle.presented)
        for _ in 0..<100{idle.setPresentation(controlsHidden:false,physicalController:true,presented:true)}
        assert(reads==stoppedReads+1) // Controller/artwork updates do not restart polling.
        print("Companion efficiency: unchanged data emits no redraw and hidden model performs no reads; covered DIY lists and scrollbar layering passed")
        print("Eden manifest: expression precedence, lazy derived values, visibility, repeats and hit testing passed")
        print("companion model: live pages, map coordinates, price slots, rewards and native text passed")
    }
}
