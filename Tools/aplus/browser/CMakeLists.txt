add_executable(odyssey
    BrowserWindow.cpp
    BrowserUI.cpp
    Delegates.cpp
    Draw.cpp
    Glyphs.cpp
    Pages.cpp
    Tab.cpp
    TextField.cpp
    main.cpp
)

target_include_directories(odyssey PRIVATE
    $<TARGET_PROPERTY:WebKitLegacy,INCLUDE_DIRECTORIES>
    "${WebCore_PRIVATE_FRAMEWORK_HEADERS_DIR}/WebCore"
    "${WEBCORE_DIR}/platform/bal"
)

target_compile_definitions(odyssey PRIVATE $<TARGET_PROPERTY:WebKitLegacy,COMPILE_DEFINITIONS> BUILDING_WITH_CMAKE=1 HAVE_CONFIG_H=1)
target_compile_options(odyssey PRIVATE -include WebKitPrefix.h)

add_dependencies(odyssey WebKitLegacy)

target_link_directories(odyssey PRIVATE ${APLUS_SDK_DIR}/lib)

target_link_libraries(odyssey PRIVATE
    -static
    -Wl,--start-group
    WebKitLegacy
    WebCore
    PAL
    JavaScriptCore
    WTF
    ${APLUS_SDK_DIR}/lib/libpsl.a
    ${APLUS_SDK_DIR}/lib/libxslt.a
    ${APLUS_SDK_DIR}/lib/libxml2.a
    ${APLUS_SDK_DIR}/lib/libicui18n.a
    ${APLUS_SDK_DIR}/lib/libicuuc.a
    ${APLUS_SDK_DIR}/lib/libicudata.a
    -lui -lulog -laudio
    -lavformat -lavcodec -lswscale -lswresample -lavutil -lbz2
    -lcurl -lssl -lcrypto
    -lcairo -lpixman-1 -lharfbuzz-icu -lharfbuzz -lfontconfig -lexpat -lfreetype
    -lwebp -lwebpdemux -ljpeg -lpng16 -lsqlite3 -lz -llzma -lzstd
    -lstdc++ -lpthread -latomic -lm
    -Wl,--end-group
)

install(TARGETS odyssey RUNTIME DESTINATION bin)
