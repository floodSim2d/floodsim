add_executable(floodsim
    main.cpp
        UI/MainWindow.h
        UI/MainWindow.cpp
)

target_link_libraries(floodsim
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Ensure moc/uic/rcc are active for this target
set_target_properties(floodsim PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

