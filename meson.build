project('fontviewer',['cpp','c'], version: '1.1.0')

executable(
    'fontviewer',
    [
        'src/main.cpp',
        'src/pangram.cpp',
        'src/googlefonts/google-fonts-window.cpp',
        'src/sushi-font-widget.c',
        'src/sushi-font-loader.c',
        'src/utils.cpp',
    ],
    dependencies: [
        dependency('fontconfig'),
        dependency('gtkmm-3.0'),
        dependency('cairomm-1.0'),
        dependency('freetype2'),
        dependency('json-glib-1.0'),
        dependency('libcurl'),
    ],
    install: true
)

install_data('data/fontviewer.desktop', install_dir: 'share/applications')
install_data('data/fontviewer-google-symbolic.svg', install_dir: 'share/icons/hicolor/scalable/actions')