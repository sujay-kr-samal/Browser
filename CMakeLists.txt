cmake_minimum_required(VERSION 3.20)

project(DivineEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(SDL3 REQUIRED CONFIG)
find_package(Freetype REQUIRED)
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(FONTCONFIG QUIET IMPORTED_TARGET fontconfig)
endif()

# Core HTML and DOM library (independent of SDL / GUI)
add_library(html_lib STATIC
    src/dom/node.cpp
    src/html/tokenizer.cpp
    src/html/parser.cpp
)
target_include_directories(html_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Core CSS tokenizer, parser, and style resolver (independent of SDL / GUI)
add_library(css_lib STATIC
    src/css/types.cpp
    src/css/stylesheet.cpp
    src/css/parser.cpp
    src/css/resolver.cpp
)
target_include_directories(css_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(css_lib PUBLIC html_lib)

# Core Layout Engine (box model, block/inline layout, geometry)
add_library(layout_lib STATIC
    src/layout/box.cpp
    src/layout/tree_builder.cpp
)
target_include_directories(layout_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(layout_lib PUBLIC html_lib css_lib)

# Painter library (painting layout tree, text, and borders to SDL3)
add_library(paint_lib STATIC
    src/paint/painter.cpp
    src/paint/text_renderer.cpp
)
target_include_directories(paint_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(paint_lib PUBLIC layout_lib SDL3::SDL3 Freetype::Freetype)
if (FONTCONFIG_FOUND)
    target_link_libraries(paint_lib PRIVATE PkgConfig::FONTCONFIG)
    target_compile_definitions(paint_lib PRIVATE DIVINE_HAVE_FONTCONFIG=1)
endif()

# Engine library (window, renderer, event loop, viewport scrolling)
add_library(engine_lib STATIC
    src/engine.cpp
)
target_include_directories(engine_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(engine_lib PUBLIC SDL3::SDL3 html_lib css_lib layout_lib paint_lib)

add_executable(divine-engine
    src/main.cpp
)
target_link_libraries(divine-engine PRIVATE engine_lib html_lib css_lib layout_lib paint_lib)

enable_testing()

# Regression tests for Phase 1 Engine and rendering
add_executable(engine_test
    tests/test_engine.cpp
)
target_link_libraries(engine_test PRIVATE engine_lib)
add_test(NAME EngineTest COMMAND engine_test)

# Phase 2 DOM and HTML Parser tests
add_executable(html_test
    tests/test_html.cpp
)
target_link_libraries(html_test PRIVATE html_lib)
add_test(NAME HtmlTest COMMAND html_test)

# Phase 2 CSS and Style Resolution tests
add_executable(css_test
    tests/test_css.cpp
)
target_link_libraries(css_test PRIVATE css_lib html_lib)
add_test(NAME CssTest COMMAND css_test)

# Phase 2 Layout Engine tests
add_executable(layout_test
    tests/test_layout.cpp
)
target_link_libraries(layout_test PRIVATE layout_lib css_lib html_lib)
add_test(NAME LayoutTest COMMAND layout_test)

# Phase 2 Painter & Viewport Scrolling tests
add_executable(paint_test
    tests/test_paint.cpp
)
target_link_libraries(paint_test PRIVATE paint_lib layout_lib css_lib html_lib SDL3::SDL3)
add_test(NAME PaintTest COMMAND paint_test)
