idf_component_register(
    SRCS
        "app_main.c"
        "app_events.c"
        "api/http_server.c"
        "api/http_guard.c"
        "api/api_routes.c"
        "api/api_layout.c"
        "api/api_entities.c"
        "api/api_state.c"
        "api/api_settings.c"
        "api/api_i18n.c"
        "api/api_wifi.c"
        "drivers/display_init.c"
        "drivers/touch_init.c"
        "ha/ha_client.c"
        "ha/ha_ws.c"
        "ha/ha_model.c"
        "layout/layout_store.c"
        "layout/layout_validate.c"
        "settings/runtime_settings.c"
        "settings/i18n_store.c"
        "net/wifi_mgr.c"
        "net/time_sync.c"
        "ui/ui_runtime.c"
        "ui/ui_boot_splash.c"
        "ui/ui_pages.c"
        "ui/ui_i18n.c"
        "ui/ui_widget_factory.c"
        "ui/ui_bindings.c"
        "ui/widgets/w_sensor.c"
        "ui/widgets/w_button.c"
        "ui/widgets/w_slider.c"
        "ui/widgets/w_graph.c"
        "ui/widgets/w_empty_tile.c"
        "ui/widgets/w_light_tile.c"
        "ui/widgets/w_heating_tile.c"
        "ui/widgets/w_weather_tile.c"
        "ui/fonts/mdi_font_registry.c"
        "ui/theme/theme_default.c"
        "util/json_util.c"
        "util/ringbuf.c"
    INCLUDE_DIRS
        "."
        "api"
        "drivers"
        "ha"
        "layout"
        "net"
        "ui"
        "ui/theme"
        "util"
    REQUIRES
        nvs_flash
        bootloader_support
        esp_netif
        esp_event
        esp_wifi
        esp_wifi_remote
        esp_hosted
        esp_websocket_client
        esp_http_client
        esp_http_server
        esp_timer
        esp_app_format
        esp_lcd
        lvgl
        json
        littlefs
        webui
)

set(APP_HAVE_HOSTED_C6_FW_IMAGE 0)
set(HOSTED_C6_FW_BIN "")
set(HOSTED_C6_FW_BIN_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../managed_components/espressif__esp_hosted/examples/host_performs_slave_ota/components/ota_partition/slave_fw_bin")
set(HOSTED_C6_FW_EMBED_DIR "${CMAKE_CURRENT_BINARY_DIR}/hosted_c6_fw")
set(HOSTED_C6_FW_BIN_TOOLS "${CMAKE_SOURCE_DIR}/tools/network_adapter_esp32c6.bin")
file(MAKE_DIRECTORY "${HOSTED_C6_FW_EMBED_DIR}")

set(HOSTED_C6_HOST_VERSION_FILE
    "${CMAKE_CURRENT_LIST_DIR}/../managed_components/espressif__esp_hosted/host/esp_hosted_host_fw_ver.h")
set(HOSTED_C6_EXPECTED_VERSION "unknown")
if(EXISTS "${HOSTED_C6_HOST_VERSION_FILE}")
    file(READ "${HOSTED_C6_HOST_VERSION_FILE}" HOSTED_C6_HOST_VERSION_TEXT)
    string(REGEX MATCH "#define[ \t]+ESP_HOSTED_VERSION_MAJOR_1[ \t]+([0-9]+)"
        _HOSTED_C6_MAJOR_MATCH "${HOSTED_C6_HOST_VERSION_TEXT}")
    set(HOSTED_C6_VER_MAJOR "${CMAKE_MATCH_1}")
    string(REGEX MATCH "#define[ \t]+ESP_HOSTED_VERSION_MINOR_1[ \t]+([0-9]+)"
        _HOSTED_C6_MINOR_MATCH "${HOSTED_C6_HOST_VERSION_TEXT}")
    set(HOSTED_C6_VER_MINOR "${CMAKE_MATCH_1}")
    string(REGEX MATCH "#define[ \t]+ESP_HOSTED_VERSION_PATCH_1[ \t]+([0-9]+)"
        _HOSTED_C6_PATCH_MATCH "${HOSTED_C6_HOST_VERSION_TEXT}")
    set(HOSTED_C6_VER_PATCH "${CMAKE_MATCH_1}")
    if(NOT HOSTED_C6_VER_MAJOR STREQUAL "" AND
       NOT HOSTED_C6_VER_MINOR STREQUAL "" AND
       NOT HOSTED_C6_VER_PATCH STREQUAL "")
        set(HOSTED_C6_EXPECTED_VERSION
            "${HOSTED_C6_VER_MAJOR}.${HOSTED_C6_VER_MINOR}.${HOSTED_C6_VER_PATCH}")
    endif()
endif()

set(HOSTED_C6_FW_BIN_MANUAL "${HOSTED_C6_FW_BIN_DIR}/network_adapter.bin")
if(NOT HOSTED_C6_EXPECTED_VERSION STREQUAL "unknown")
    set(HOSTED_C6_FW_BIN_EXPECTED
        "${HOSTED_C6_FW_BIN_DIR}/network_adapter_${HOSTED_C6_EXPECTED_VERSION}.bin")
endif()
set(HOSTED_C6_FW_BIN_LOCAL_BUILD "${CMAKE_SOURCE_DIR}/build_c6/network_adapter.bin")
set(HOSTED_C6_SLAVE_PROJECT_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../managed_components/espressif__esp_hosted/slave")
set(HOSTED_C6_SLAVE_BUILD_DIR "${CMAKE_SOURCE_DIR}/build_c6")
set(HOSTED_C6_SLAVE_BIN "${HOSTED_C6_SLAVE_BUILD_DIR}/network_adapter.bin")

if(DEFINED HOSTED_C6_FW_BIN_EXPECTED AND EXISTS "${HOSTED_C6_FW_BIN_EXPECTED}")
    set(HOSTED_C6_FW_BIN "${HOSTED_C6_FW_BIN_EXPECTED}")
endif()

# Prefer a source-built C6 image from the official esp_hosted slave project when
# a version-matched cached image is not available yet.
if(NOT HOSTED_C6_FW_BIN AND CONFIG_ESP_HOSTED_ENABLED)
    if(EXISTS "${HOSTED_C6_SLAVE_PROJECT_DIR}/CMakeLists.txt")
        idf_build_get_property(PYTHON PYTHON)
        set(HOSTED_C6_IDF_PY "$ENV{IDF_PATH}/tools/idf.py")
        if(PYTHON AND EXISTS "${HOSTED_C6_IDF_PY}")
            message(STATUS
                "Building ESP-Hosted C6 firmware from source: ${HOSTED_C6_SLAVE_PROJECT_DIR} (target=esp32c6)")
            execute_process(
                COMMAND "${CMAKE_COMMAND}" -E env
                    "IDF_TARGET=esp32c6"
                    "${PYTHON}" "${HOSTED_C6_IDF_PY}"
                    -C "${HOSTED_C6_SLAVE_PROJECT_DIR}"
                    -B "${HOSTED_C6_SLAVE_BUILD_DIR}"
                    -DIDF_TARGET=esp32c6
                    build
                RESULT_VARIABLE HOSTED_C6_BUILD_RESULT
                OUTPUT_VARIABLE HOSTED_C6_BUILD_OUTPUT
                ERROR_VARIABLE HOSTED_C6_BUILD_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE
            )
            if(HOSTED_C6_BUILD_RESULT EQUAL 0 AND EXISTS "${HOSTED_C6_SLAVE_BIN}")
                file(MAKE_DIRECTORY "${HOSTED_C6_FW_BIN_DIR}")
                if(HOSTED_C6_EXPECTED_VERSION STREQUAL "unknown")
                    set(HOSTED_C6_FW_BIN_AUTO "${HOSTED_C6_FW_BIN_DIR}/network_adapter_autobuild.bin")
                else()
                    set(HOSTED_C6_FW_BIN_AUTO
                        "${HOSTED_C6_FW_BIN_DIR}/network_adapter_${HOSTED_C6_EXPECTED_VERSION}.bin")
                endif()
                configure_file("${HOSTED_C6_SLAVE_BIN}" "${HOSTED_C6_FW_BIN_AUTO}" COPYONLY)
                set(HOSTED_C6_FW_BIN "${HOSTED_C6_FW_BIN_AUTO}")
                message(STATUS "Built and cached ESP-Hosted C6 firmware image: ${HOSTED_C6_FW_BIN_AUTO}")
            else()
                message(WARNING
                    "Automatic C6 firmware build failed (result=${HOSTED_C6_BUILD_RESULT}). "
                    "Falling back to prebuilt firmware selection.")
                if(HOSTED_C6_BUILD_ERROR)
                    message(WARNING "C6 build stderr: ${HOSTED_C6_BUILD_ERROR}")
                endif()
            endif()
        else()
            message(WARNING
                "Cannot auto-build C6 firmware: idf.py or python path is unavailable in CMake context.")
        endif()
    else()
        message(WARNING
            "Cannot auto-build C6 firmware: missing slave project at ${HOSTED_C6_SLAVE_PROJECT_DIR}")
    endif()
endif()

if(NOT HOSTED_C6_FW_BIN)
    if(DEFINED HOSTED_C6_FW_BIN_EXPECTED AND EXISTS "${HOSTED_C6_FW_BIN_EXPECTED}")
        set(HOSTED_C6_FW_BIN "${HOSTED_C6_FW_BIN_EXPECTED}")
    elseif(NOT HOSTED_C6_EXPECTED_VERSION STREQUAL "unknown")
        message(WARNING
            "No version-matched C6 firmware image available for host version ${HOSTED_C6_EXPECTED_VERSION}. "
            "Refusing unversioned fallbacks to avoid embedding incompatible slave firmware.")
    elseif(EXISTS "${HOSTED_C6_FW_BIN_TOOLS}")
        set(HOSTED_C6_FW_BIN "${HOSTED_C6_FW_BIN_TOOLS}")
        message(WARNING
            "Using tools/network_adapter_esp32c6.bin as fallback because host version is unknown.")
    elseif(EXISTS "${HOSTED_C6_FW_BIN_MANUAL}")
        set(HOSTED_C6_FW_BIN "${HOSTED_C6_FW_BIN_MANUAL}")
        message(WARNING
            "Using managed-components generic network_adapter.bin fallback because host version is unknown.")
    elseif(EXISTS "${HOSTED_C6_FW_BIN_LOCAL_BUILD}")
        set(HOSTED_C6_FW_BIN "${HOSTED_C6_FW_BIN_LOCAL_BUILD}")
        message(WARNING
            "Using build_c6/network_adapter.bin fallback because host version is unknown.")
    else()
        file(GLOB HOSTED_C6_FW_BIN_CANDIDATES "${HOSTED_C6_FW_BIN_DIR}/*.bin")
        list(LENGTH HOSTED_C6_FW_BIN_CANDIDATES HOSTED_C6_FW_BIN_COUNT)
        if(HOSTED_C6_FW_BIN_COUNT GREATER 0)
            list(SORT HOSTED_C6_FW_BIN_CANDIDATES ORDER DESCENDING)
            list(GET HOSTED_C6_FW_BIN_CANDIDATES 0 HOSTED_C6_FW_BIN)
            message(WARNING
                "Using best-effort C6 firmware fallback because host version is unknown: ${HOSTED_C6_FW_BIN}")
        endif()
    endif()
endif()

if(HOSTED_C6_FW_BIN)
    set(HOSTED_C6_FW_EMBED "${HOSTED_C6_FW_EMBED_DIR}/hosted_c6_fw.bin")
    configure_file("${HOSTED_C6_FW_BIN}" "${HOSTED_C6_FW_EMBED}" COPYONLY)
    target_add_binary_data(${COMPONENT_LIB} "${HOSTED_C6_FW_EMBED}" BINARY)
    set(APP_HAVE_HOSTED_C6_FW_IMAGE 1)
    message(STATUS "Embedding ESP-Hosted C6 firmware image: ${HOSTED_C6_FW_BIN}")
else()
    message(WARNING
        "No ESP-Hosted C6 firmware image found in ${HOSTED_C6_FW_BIN_DIR}; "
        "runtime C6 auto-update will be skipped. "
        "Either provide network_adapter.bin there or allow auto-build to succeed.")
endif()

target_compile_definitions(${COMPONENT_LIB} PRIVATE APP_HAVE_HOSTED_C6_FW_IMAGE=${APP_HAVE_HOSTED_C6_FW_IMAGE})

set(SPLASH_HOUSE_IMAGE_SOURCE "${CMAKE_CURRENT_LIST_DIR}/ui/assets/splash_house_image.c")
if(EXISTS "${SPLASH_HOUSE_IMAGE_SOURCE}")
    target_sources(${COMPONENT_LIB} PRIVATE "${SPLASH_HOUSE_IMAGE_SOURCE}")
endif()

set(SPLASH_SMART86_BETTA_SOURCE "${CMAKE_CURRENT_LIST_DIR}/ui/assets/SMART86OS_Betta.c")
set(SPLASH_SMART86_BETTA_AVAILABLE 0)
if(EXISTS "${SPLASH_SMART86_BETTA_SOURCE}")
    target_sources(${COMPONENT_LIB} PRIVATE "${SPLASH_SMART86_BETTA_SOURCE}")
    set(SPLASH_SMART86_BETTA_AVAILABLE 1)
endif()
target_compile_definitions(${COMPONENT_LIB} PRIVATE APP_HAVE_SMART86OS_BETTA_IMAGE=${SPLASH_SMART86_BETTA_AVAILABLE})

set(MDI_TOP50_FONT72_SOURCE "${CMAKE_CURRENT_LIST_DIR}/ui/fonts/mditop50icons72.c")
set(MDI_TOP50_FONT56_SOURCE "${CMAKE_CURRENT_LIST_DIR}/ui/fonts/mditop50icons56.c")
set(MDI_TOP50_FONT_SOURCE "${CMAKE_CURRENT_LIST_DIR}/ui/fonts/mditop50icons.c")
set(MDI_STANDARD_FONT_SOURCE "${CMAKE_CURRENT_LIST_DIR}/ui/fonts/mdi_standard_icons_56.c")
set(MDI_WEATHER_FONT120_SOURCE "${CMAKE_CURRENT_LIST_DIR}/ui/fonts/mdiweatherIcons120.c")
set(MDI_WEATHER_FONT100_SOURCE "${CMAKE_CURRENT_LIST_DIR}/ui/fonts/mdiweatherIcons100.c")
set(MDI_WEATHER_FONT20_SOURCE "${CMAKE_CURRENT_LIST_DIR}/ui/fonts/mdiweatherIcons20.c")

set(MDI_TOP50_FONT72_AVAILABLE 0)
if(EXISTS "${MDI_TOP50_FONT72_SOURCE}")
    target_sources(${COMPONENT_LIB} PRIVATE "${MDI_TOP50_FONT72_SOURCE}")
    set(MDI_TOP50_FONT72_AVAILABLE 1)
endif()

set(MDI_TOP50_FONT56_AVAILABLE 0)
if(EXISTS "${MDI_TOP50_FONT56_SOURCE}")
    target_sources(${COMPONENT_LIB} PRIVATE "${MDI_TOP50_FONT56_SOURCE}")
    set(MDI_TOP50_FONT56_AVAILABLE 1)
endif()

set(MDI_TOP50_FONT42_AVAILABLE 0)

set(MDI_WEATHER_FONT120_AVAILABLE 0)
if(EXISTS "${MDI_WEATHER_FONT120_SOURCE}")
    target_sources(${COMPONENT_LIB} PRIVATE "${MDI_WEATHER_FONT120_SOURCE}")
    set(MDI_WEATHER_FONT120_AVAILABLE 1)
endif()

set(MDI_WEATHER_FONT100_AVAILABLE 0)
if(EXISTS "${MDI_WEATHER_FONT100_SOURCE}")
    target_sources(${COMPONENT_LIB} PRIVATE "${MDI_WEATHER_FONT100_SOURCE}")
    set(MDI_WEATHER_FONT100_AVAILABLE 1)
endif()

set(MDI_WEATHER_FONT20_AVAILABLE 0)
if(EXISTS "${MDI_WEATHER_FONT20_SOURCE}")
    target_sources(${COMPONENT_LIB} PRIVATE "${MDI_WEATHER_FONT20_SOURCE}")
    set(MDI_WEATHER_FONT20_AVAILABLE 1)
endif()

if(MDI_TOP50_FONT42_AVAILABLE OR MDI_TOP50_FONT56_AVAILABLE OR MDI_TOP50_FONT72_AVAILABLE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE APP_HAVE_MDI_ICON_FONT=1 APP_HAVE_MDI_TOP50_FONT=1 APP_HAVE_MDI_TOP50_FONT_72=${MDI_TOP50_FONT72_AVAILABLE} APP_HAVE_MDI_TOP50_FONT_56=${MDI_TOP50_FONT56_AVAILABLE} APP_HAVE_MDI_TOP50_FONT_42=${MDI_TOP50_FONT42_AVAILABLE} APP_HAVE_MDI_TOP50_FONT_LEGACY=0 APP_HAVE_MDI_STANDARD_FONT=0 APP_HAVE_MDI_WEATHER_FONT_120=${MDI_WEATHER_FONT120_AVAILABLE} APP_HAVE_MDI_WEATHER_FONT_100=${MDI_WEATHER_FONT100_AVAILABLE} APP_HAVE_MDI_WEATHER_FONT_20=${MDI_WEATHER_FONT20_AVAILABLE})
elseif(EXISTS "${MDI_TOP50_FONT_SOURCE}")
    target_sources(${COMPONENT_LIB} PRIVATE "${MDI_TOP50_FONT_SOURCE}")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE APP_HAVE_MDI_ICON_FONT=1 APP_HAVE_MDI_TOP50_FONT=1 APP_HAVE_MDI_TOP50_FONT_72=0 APP_HAVE_MDI_TOP50_FONT_56=0 APP_HAVE_MDI_TOP50_FONT_42=0 APP_HAVE_MDI_TOP50_FONT_LEGACY=1 APP_HAVE_MDI_STANDARD_FONT=0 APP_HAVE_MDI_WEATHER_FONT_120=${MDI_WEATHER_FONT120_AVAILABLE} APP_HAVE_MDI_WEATHER_FONT_100=${MDI_WEATHER_FONT100_AVAILABLE} APP_HAVE_MDI_WEATHER_FONT_20=${MDI_WEATHER_FONT20_AVAILABLE})
elseif(EXISTS "${MDI_STANDARD_FONT_SOURCE}")
    target_sources(${COMPONENT_LIB} PRIVATE "${MDI_STANDARD_FONT_SOURCE}")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE APP_HAVE_MDI_ICON_FONT=1 APP_HAVE_MDI_TOP50_FONT=0 APP_HAVE_MDI_TOP50_FONT_72=0 APP_HAVE_MDI_TOP50_FONT_56=0 APP_HAVE_MDI_TOP50_FONT_42=0 APP_HAVE_MDI_TOP50_FONT_LEGACY=0 APP_HAVE_MDI_STANDARD_FONT=1 APP_HAVE_MDI_WEATHER_FONT_120=${MDI_WEATHER_FONT120_AVAILABLE} APP_HAVE_MDI_WEATHER_FONT_100=${MDI_WEATHER_FONT100_AVAILABLE} APP_HAVE_MDI_WEATHER_FONT_20=${MDI_WEATHER_FONT20_AVAILABLE})
else()
    target_compile_definitions(${COMPONENT_LIB} PRIVATE APP_HAVE_MDI_ICON_FONT=0 APP_HAVE_MDI_TOP50_FONT=0 APP_HAVE_MDI_TOP50_FONT_72=0 APP_HAVE_MDI_TOP50_FONT_56=0 APP_HAVE_MDI_TOP50_FONT_42=0 APP_HAVE_MDI_TOP50_FONT_LEGACY=0 APP_HAVE_MDI_STANDARD_FONT=0 APP_HAVE_MDI_WEATHER_FONT_120=${MDI_WEATHER_FONT120_AVAILABLE} APP_HAVE_MDI_WEATHER_FONT_100=${MDI_WEATHER_FONT100_AVAILABLE} APP_HAVE_MDI_WEATHER_FONT_20=${MDI_WEATHER_FONT20_AVAILABLE})
endif()

set(APP_UI_WEATHER_LOTTIE_ASSETS 0)
if(CONFIG_LV_USE_LOTTIE)
    set(APP_UI_WEATHER_LOTTIE_ASSETS 1)
    set(WEATHER_LOTTIE_DIR "${CMAKE_CURRENT_LIST_DIR}/ui/weather_icons/fill/lottie")
    set(WEATHER_LOTTIE_EMBEDS
        "clear-day:clear_day"
        "clear-night:clear_night"
        "cloudy:cloudy"
        "fog:fog"
        "hail:hail"
        "partly-cloudy-day:partly_cloudy_day"
        "partly-cloudy-night:partly_cloudy_night"
        "sleet:sleet"
        "wind:wind"
        "overcast-day:overcast_day"
        "extreme:extreme"
        "extreme-rain:extreme_rain"
        "rain:rain"
        "snow:snow"
        "thunderstorms:thunderstorms"
        "thunderstorms-rain:thunderstorms_rain")

    foreach(WEATHER_LOTTIE_PAIR IN LISTS WEATHER_LOTTIE_EMBEDS)
        string(REPLACE ":" ";" WEATHER_LOTTIE_PARTS "${WEATHER_LOTTIE_PAIR}")
        list(GET WEATHER_LOTTIE_PARTS 0 WEATHER_LOTTIE_SRC_STEM)
        list(GET WEATHER_LOTTIE_PARTS 1 WEATHER_LOTTIE_DST_STEM)

        set(WEATHER_LOTTIE_SRC_FILE "${WEATHER_LOTTIE_DIR}/${WEATHER_LOTTIE_SRC_STEM}.json")
        set(WEATHER_LOTTIE_EMBED_FILE "${CMAKE_CURRENT_BINARY_DIR}/weather_${WEATHER_LOTTIE_DST_STEM}.json")
        if(EXISTS "${WEATHER_LOTTIE_SRC_FILE}")
            configure_file("${WEATHER_LOTTIE_SRC_FILE}" "${WEATHER_LOTTIE_EMBED_FILE}" COPYONLY)
            target_add_binary_data(${COMPONENT_LIB} "${WEATHER_LOTTIE_EMBED_FILE}" TEXT)
        else()
            set(APP_UI_WEATHER_LOTTIE_ASSETS 0)
            message(WARNING "Missing weather lottie asset: ${WEATHER_LOTTIE_SRC_FILE}")
        endif()
    endforeach()
endif()

target_compile_definitions(${COMPONENT_LIB} PRIVATE APP_UI_WEATHER_LOTTIE_ASSETS=${APP_UI_WEATHER_LOTTIE_ASSETS})

# Keep strict warnings in project code, but silence an upstream ThorVG warning in lvgl.
if(TARGET __idf_lvgl__lvgl)
    target_compile_options(__idf_lvgl__lvgl PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-type-limits>)
endif()
