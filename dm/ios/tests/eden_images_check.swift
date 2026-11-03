// Appended to the renderer in this test target so production privacy stays intact.
extension EdenImages {
    func discardWarmImagesForTest(){cache.removeAllObjects()}
    func seedFailureForTest(_ key:String,_ attempts:Int,_ age:TimeInterval){failed[key]=Date().addingTimeInterval(-age);self.attempts[key]=attempts}
    var retryDelayForTest:Double?{retryAt?.timeIntervalSinceNow}
    var failureCountForTest:Int{failed.count}
}
@main struct EdenImagesCheck {
    @MainActor static func settle() async {for _ in 0..<8{await Task.yield()};try? await Task.sleep(for:.milliseconds(10))}
    @MainActor static func main() async {
        var loads=0
        var delayed:((Data?,Int,Int)->Void)?
        let pixels=Data([255,0,0,255])
        let model=CompanionModel(reader:{["acnh.ready":1]},sender:{_,_ in},images:{key,done in
            loads+=1
            if key=="module:delayed"{delayed=done}
            else if key.hasPrefix("module:bad") || key=="module:fast"{done(nil,0,0)}
            else{done(pixels,1,1)}
        })
        let images=EdenImages()
        images.request(["module:first"],model);await settle()
        assert(images.get("module:first") != nil)
        images.discardWarmImagesForTest()
        assert(images.get("module:first") != nil)
        let loaded=loads
        images.request(["module:first"],model);await settle();assert(loads==loaded)
        images.request(["module:second"],model);await settle();images.discardWarmImagesForTest()
        assert(images.get("module:first")==nil && images.get("module:second") != nil)
        model.visible(false);images.suspend();images.discardWarmImagesForTest()
        assert(images.get("module:second")==nil)
        model.visible(true);images.request(["module:second"],model);await settle()
        let beforeTrim=loads
        images.trim();assert(images.get("module:second")==nil)
        images.request(["module:second"],model);await settle()
        assert(loads==beforeTrim+1 && images.get("module:second") != nil)
        images.request(["module:delayed"],model)
        images.request(["module:second"],model)
        delayed?(pixels,1,1);delayed=nil;await settle();images.discardWarmImagesForTest()
        assert(images.get("module:delayed")==nil && images.get("module:second") != nil)
        for i in 0..<32{images.request(["module:bad\(i)"],model);await settle();assert(images.failureCountForTest<=1)}
        images.trim();images.seedFailureForTest("module:slow",5,0)
        images.request(["module:slow","module:fast"],model);await settle()
        assert((images.retryDelayForTest ?? 99)>1 && (images.retryDelayForTest ?? 99)<3)
        // A cancelled earlier callback cannot erase a newly scheduled retry.
        images.trim();images.seedFailureForTest("module:old",5,29.95)
        images.request(["module:old"],model)
        images.trim();images.seedFailureForTest("module:new",1,0)
        images.request(["module:new"],model)
        try? await Task.sleep(for:.milliseconds(120))
        assert((images.retryDelayForTest ?? 0)>1)
        // An expired failure that is already loading needs no 50-ms polling loop.
        images.trim();images.seedFailureForTest("module:delayed",1,3)
        images.request(["module:delayed"],model)
        assert(images.retryDelayForTest==nil)
        delayed?(nil,0,0);delayed=nil;await settle()
        assert((images.retryDelayForTest ?? 0)>3)
        model.visible(false);images.suspend();assert(images.retryDelayForTest==nil)
        print("companion images: visible pins, eviction, hide, trim, stale completion, bounded failure scope and retry generations passed")
    }
}
