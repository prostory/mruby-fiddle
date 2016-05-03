MRuby::Gem::Specification.new('mruby-fiddle') do |spec|
  spec.license = 'MIT'
  spec.author  = 'Xiao peng'

  # Add compile flags
  spec.cc.flags << '-g -DHAVE_DLFCN_H -DHAVE_DLERROR -DUSE_FFI_CLOSURE_ALLOC' #-DMEMORY_TRACE -DMEMORY_INFO

  # Add cflags to all
  spec.mruby.cc.flags << '-g'

  # Add libraries
  spec.linker.libraries << ['dl', 'ffi']

  # Add dependency
  spec.add_dependency('mruby-error')

  spec.rbfiles = [
      "#{dir}/mrblib/closure.rb",
      "#{dir}/mrblib/function.rb",
      "#{dir}/mrblib/fiddle.rb",
      "#{dir}/mrblib/cparser.rb",
      "#{dir}/mrblib/types.rb",
      "#{dir}/mrblib/value.rb",
      "#{dir}/mrblib/pack.rb",
      "#{dir}/mrblib/struct.rb",
      "#{dir}/mrblib/import.rb"
  ]

  # Default build files
  #spec.rbfiles = Dir.glob("#{dir}/mrblib/*.rb")
  #spec.objs = Dir.glob("#{dir}/src/*.{c,cpp,m,asm,S}").map { |f| objfile(f.relative_path_from(dir).pathmap("#{build_dir}/%X")) }
  # spec.test_rbfiles = Dir.glob("#{dir}/test/*.rb")
  # spec.test_objs = Dir.glob("#{dir}/test/*.{c,cpp,m,asm,S}").map { |f| objfile(f.relative_path_from(dir).pathmap("#{build_dir}/%X")) }
  # spec.test_preload = 'test/assert.rb'

  # Values accessible as TEST_ARGS inside test scripts
  # spec.test_args = {'tmp_dir' => Dir::tmpdir}
end
