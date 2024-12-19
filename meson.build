project('fontviewer',['cpp','c'])

executable(
    'fontviewer',
    [
        'src/main.cpp',
        'src/sushi-font-widget.c',
        'src/sushi-font-loader.c',
    ],
    dependencies: [
        dependency('fontconfig'),
        dependency('gtkmm-3.0'),
        dependency('cairomm-1.0'),
        dependency('freetype2'),
    ],
    install: true
)

install_data('data/fontviewer.desktop', install_dir: 'share/applications')