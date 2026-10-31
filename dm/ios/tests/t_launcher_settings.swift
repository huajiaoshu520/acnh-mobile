import Foundation

@main struct LauncherSettingsRegression {
 static func main() throws {
  let rows = """
  {"settings":[
   {"env":"ACNH_CONTROLLER_LAYOUT","kind":"choice","section":"Controls","label":"Layout","note":"","default":"acnh","options":[["generic","Generic"],["acnh","Dedicated"],["custom","Custom"]]},
   {"env":"ACNH_CUSTOM_BASE","kind":"choice","section":"Controls","label":"Customize from","note":"","default":"acnh","needs":"ACNH_CONTROLLER_LAYOUT=custom","options":[["generic","Generic"],["acnh","Dedicated"]]},
   {"env":"DM_FPS_HOLD","kind":"toggle","section":"Performance","label":"Hold","note":"","default":"on","on":"on","off":"off","available":false},
   {"env":"DM_RUMBLE_GAIN","kind":"range","section":"Controls","label":"Gain","note":"","default":"1.0","min":0,"max":3,"step":0.1},
   {"env":"DM_NVN_ENC_THREAD","kind":"toggle","section":"Performance","label":"Encode","note":"","default":"1"}]}
  """
  let settings = try JSONDecoder().decode(SettingsFile.self, from: Data(rows.utf8)).settings
  let suite = "org.acnhmobile.launcher-regression.\(UUID().uuidString)"
  let store = UserDefaults(suiteName:suite)!
  defer { store.removePersistentDomain(forName:suite) }
  store.set("invalid",forKey:"launcher.DM_FPS_HOLD")
  store.set("nan",forKey:"launcher.DM_RUMBLE_GAIN")
  let oldKeys = getenv("DM_ENV_FILE_KEYS").map{String(cString:$0)}
  let oldThread = getenv("DM_NVN_ENC_THREAD").map{String(cString:$0)}
  defer {
   if let v=oldKeys {setenv("DM_ENV_FILE_KEYS",v,1)} else {unsetenv("DM_ENV_FILE_KEYS")}
   if let v=oldThread {setenv("DM_NVN_ENC_THREAD",v,1)} else {unsetenv("DM_NVN_ENC_THREAD")}
  }
  setenv("DM_ENV_FILE_KEYS","DM_NVN_ENC_THREAD",1);setenv("DM_NVN_ENC_THREAD","0",1)
  let model=LauncherModel(settings,defaults:store)
  precondition(model.values["DM_FPS_HOLD"]=="on")
  precondition(model.values["DM_RUMBLE_GAIN"]=="1.0")
  precondition(model.values["DM_NVN_ENC_THREAD"]=="0")
  model.values["DM_RUMBLE_GAIN"]="1.00";precondition(!model.changed)
  model.values["DM_RUMBLE_GAIN"]="2.0";precondition(model.changed)
  model.reset();precondition(!model.changed)
  precondition(model.values["DM_NVN_ENC_THREAD"]=="0")
  precondition(settings[2].available==false && settings[2].onValue=="on" && settings[2].offValue=="off")
  let custom = settings[1]
  for layout in ["generic", "acnh", "custom", "generic", "custom", "acnh"] {
   model.values["ACNH_CONTROLLER_LAYOUT"] = layout
   precondition(model.visible(custom) == (layout == "custom"))
   precondition(model.rows(in:"Controls").contains(where:{$0.env == custom.env}) == (layout == "custom"))
  }
  print("launcher model: validation, typed toggle, numeric defaults, reset and env locks passed")
 }
}
