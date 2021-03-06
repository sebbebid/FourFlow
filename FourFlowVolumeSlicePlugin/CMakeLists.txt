FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

# create a plugin with a custom view that shows up in ParaView's multi-view
# manager.  this plugin also contains a custom display panel

# moc the Qt based .h files
QT4_WRAP_CPP(MOC_SRCS 
	VolumeSliceView.h 
	#VolumeSliceDisplay.h
)

SET( APP_RESOURCES
  ./resources.qrc
)

QT4_ADD_RESOURCES( APP_RESOURCES_OUT 
  ${APP_RESOURCES} 
)


# invoke macro to create sources for our custom view and display panel
ADD_PARAVIEW_VIEW_MODULE(
                         # returns the interfaces defined (pass in
                         # GUI_INTERFACES parameter)
                         IFACES  
                         # returns a list of source files for this interface
                         IFACE_SRCS 
                         # give the view type 
                         # With VolumeSliceView.h implementing a
                         # pqGenericViewModule and VolumeSliceView being the XML name
                         # for the view on the server side
                         VIEW_TYPE VolumeSliceView 
                         # the XML group of the view in the server manager xml
                         VIEW_XML_GROUP views
                         # the XML name of the display for this view
                         #DISPLAY_XML VolumeSliceDisplay 
                         # the name of the display panel for this display
                         # With VolumeSliceDisplay.h implementing pqDisplayPanel
                         #DISPLAY_PANEL VolumeSliceDisplay
)

#ADD_PARAVIEW_VIEW_MODULE(
#    VIEW_IFACE IFACE_SRCS
#    VIEW_TYPE VolumeSliceView
#    VIEW_NAME VolumeSliceDisplay
#    VIEW_XML_GROUP views
#    VIEW_XML_NAME VolumeSliceDisplay
#)

QT4_WRAP_UI(UI_BUILT_SOURCES
  probeResultWindow.ui
)

#ADD_PARAVIEW_VIEW_OPTIONS(OPTIONS_IFACE OPTIONS_IFACE_SRCS VIEW_TYPE VolumeSliceView ACTIVE_VIEW_OPTIONS)

# Create a plugin with the new view and its options.
SET(PLUGIN_NAME vtkFourFlowVolumeSliceViewPlugin)
ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} "1.0" 
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVVolumeSliceView.cxx
	vtkSMVolumeSliceViewProxy.cxx
  SERVER_MANAGER_XML 
	VolumeSliceViewSM.xml
  GUI_INTERFACES 
	${IFACES} 
	#${OPTIONS_IFACE}
  GUI_SOURCES 
	VolumeSliceView.cxx 
	#VolumeSliceDisplay.cxx 
	VolumeSliceWidget.cxx 
    VolumeSliceWidget.h 
	${MOC_SRCS} 
	${IFACE_SRCS} 
    #${OPTIONS_IFACE_SRCS} 
	${UI_BUILT_SOURCES} 
	${APP_RESOURCES_OUT}
)


#vtkCommon 
TARGET_LINK_LIBRARIES(${PLUGIN_NAME} vtkPVCommon pqCore pqComponents pqApplicationComponents
  ${pv_common}
  ${vtkWidgets}
)