def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  conf.check(lib=['tesseract'], uselib_store='TESSERACT')
  conf.check(lib=['lept'], uselib_store='LEPTONICA')
  
def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-Wall", "-fPIC", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-D_GNU_SOURCE"]
  obj.target = "tesseract_native"
  obj.source = "node-tesseract-native.cc"
  obj.uselib = ['TESSERACT', 'LEPTONICA']