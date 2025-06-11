project('fontviewer', ['cpp', 'c'], version: '1.1.0')

prefix = get_option('prefix')
datadir = get_option('datadir')

subdir('po')

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
    install: true,
    cpp_args: ['-DDATADIR="' + join_paths(prefix, datadir) + '"']
)

install_data('data/fontviewer.desktop', install_dir: 'share/applications')
install_data(
    'data/fontviewer-google-symbolic.svg',
    install_dir: 'share/icons/hicolor/scalable/actions',
)
install_data(
    'data/fontviewer-google-colorful.svg',
    install_dir: 'share/icons/hicolor/scalable/actions',
)