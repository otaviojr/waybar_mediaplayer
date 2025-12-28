project(
    'waybar_mediaplayer', 'c',
    version: '0.1.0',
    license: 'MIT',
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

glib_dep     = dependency('glib-2.0', required : true)
gobject_dep  = dependency('gobject-2.0', required : true)
gio_dep      = dependency('gio-2.0', required : true)

glib_deps = [
  glib_dep,
  gobject_dep,
  gio_dep,
]

shared_library('waybar_mediaplayer',
    ['main.c','media_controller.c','mpris_media_player.c', 'mpris_media_manager.c'],
    dependencies: [
        m_dep,
        dependency('gtk+-3.0', version : ['>=3.22.0']),
        dependency('pango', version: '>=1.50'),
        dependency('cairo', version: '>=1.17')
    ] + glib_deps,
    name_prefix: ''
)
