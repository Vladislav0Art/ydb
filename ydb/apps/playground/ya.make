PROGRAM(playground)

IF(BUILD_TYPE == RELEASE)
    STRIP()
ENDIF()

SRCS(
    main.cpp
)

PEERDIR(
    ydb/apps/playground/components
    library/cpp/string_utils/csv
    util
    # util/folder
    # util/generic
    # util/system
)


IF (NOT USE_SSE4 AND NOT OPENSOURCE)
    # contrib/libs/glibasm can not be built without SSE4
    # Replace it with contrib/libs/asmlib which can be built this way.
    DISABLE(USE_ASMLIB)
    PEERDIR(
        contrib/libs/asmlib
    )
ENDIF()

END()
