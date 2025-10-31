project('ubinutils', ['c'],
  default_options: [
    'c_std=gnu11',
    'wrap_mode=forcefallback',
  ])

cmake = import('cmake')
capstone = cmake.subproject('capstone')
capstone_dep = capstone.dependency('capstone')

src = [
  './src/aof.c',
  './src/ar.c',
  './src/chunkfile.c',
  './src/elf.c',
  './src/pe.c',
  './src/arch.c',
  './src/utils.c',
]

headers = [
  './include/ubu/aof.h',
  './include/ubu/ar.h',
  './include/ubu/chunkfile.h',
  './include/ubu/elf.h',
  './include/ubu/memfile.h',
  './include/ubu/arch.h',
]

ubu = static_library(
  'ubu',
  sources: src,
  install: true,
  include_directories: ['include'],
  dependencies: [capstone_dep])

ubu_dep = declare_dependency(
  include_directories: ['include'],
  link_with: ubu)

install_headers(headers, install_dir: 'include/ubu/')


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
