set(MEDIT_SUPPORTED_SYSTEMS "Windows" "Linux")
if(NOT CMAKE_SYSTEM_NAME IN_LIST MEDIT_SUPPORTED_SYSTEMS)
    list(JOIN MEDIT_SUPPORTED_SYSTEMS ", " MEDIT_SUPPORTED_SYSTEMS_STR)
    message(
        " Target system `${CMAKE_SYSTEM_NAME}` is currently not supported.\n"
        " Supported target systems:\n"
        "   ${MEDIT_SUPPORTED_SYSTEMS_STR}."
    )
endif()
