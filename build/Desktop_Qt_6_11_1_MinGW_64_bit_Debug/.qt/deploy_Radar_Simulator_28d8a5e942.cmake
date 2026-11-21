include("C:/Qt/My_Project/Radar_Simulator/build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Radar_Simulator-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/Qt/My_Project/Radar_Simulator/build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug/Radar_Simulator.exe"
    GENERATE_QT_CONF
)
