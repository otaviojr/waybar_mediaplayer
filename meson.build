project(
    'waybar_mediaplayer', 'c',
    version: '0.1.0',
    license: 'MIT',
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

shared_library('waybar_mediaplayer',
    ['main.c','media_player.c','media_controller.c'],
    dependencies: [
        m_dep,
        dependency('gtk+-3.0', version : ['>=3.22.0']),
        dependency('playerctl', version : ['>=2.4.1']),
        dependency('pango', version: '>=1.50'),
        dependency('cairo', version: '>=1.17')
    ],
    name_prefix: ''
)
