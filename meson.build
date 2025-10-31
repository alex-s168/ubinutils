project('ubinutils', ['c'],
  default_options: [
    'c_std=gnu11',
    'wrap_mode=forcefallback',
  ])

src = [
  './lib/aof.c',
  './lib/ar.c',
  './lib/chunkfile.c',
  './lib/elf.c',
  './lib/pe.c',
]

headers = [
  './lib/aof.h',
  './lib/ar.h',
  './lib/chunkfile.h',
  './lib/elf.h',
  './lib/memfile.h',
]

ubu = static_library(
  'ubu',
  sources: src,
  install: true)

ubu_dep = declare_dependency(
  include_directories: ['lib'],
  link_with: ubu)

install_headers(headers, install_dir: 'include/ubu/')

cmake = import('cmake')
capstone = cmake.subproject('capstone')
capstone_dep = capstone.dependency('capstone')

if not meson.is_subproject()
  executable('ar',
    sources     : ['./tools/ar.c'],
    dependencies: [ubu_dep])

  executable('nm',
    sources     : ['./tools/nm.c'],
    dependencies: [ubu_dep])

  executable('objinfo',
    sources     : ['./tools/objinfo.c'],
    dependencies: [ubu_dep])

  executable('size',
    sources     : ['./tools/size.c'],
    dependencies: [ubu_dep])

  executable('flatdis',
    sources     : ['./tools/flatdis.c'],
    dependencies: [ubu_dep, capstone_dep])
endif
