if (UseQt5)
	find_package(Qt5 REQUIRED COMPONENTS Widgets)
	list(APPEND QT_LIBRARIES Qt5::Widgets)
else()
	find_package(Qt5 REQUIRED COMPONENTS QtGui)
	list(APPEND QT_LIBRARIES Qt4::QtGui)
endif()

set(CMAKE_AUTOUIC ON)

add_executable(filesystem_model filesystem_model.cpp filesystem_model.ui)
target_link_libraries(filesystem_model qtssh ${QT_LIBRARIES})
