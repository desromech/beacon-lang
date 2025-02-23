set(BeaconVM_Sources
    Context.c
    Exceptions.c
    Memory.c
    ArrayList.c
    Dictionary.c
    SourceCode.c
    Scanner.c
    Parser.c
    Bytecode.c
    SyntaxCompiler.c
)

if(SDL2_LIBRARY AND SDL2_INCLUDE_DIR)
    set(BeaconVM_SDL2Sources
        SDL2Window.c
    )

    add_library(BeaconVMCore ${BeaconVM_Sources} ${BeaconVM_SDL2Sources})
    target_link_libraries(BeaconVMCore ${SDL2_LIBRARY})

elseif(XCB AND NOT APPLE)
    set(BeaconVM_XcbSources
        XcbWindow.c
    )

    add_library(BeaconVMCore ${BeaconVM_Sources} ${BeaconVM_XcbSources})
    target_link_libraries(BeaconVMCore ${XCB})
else()
    set(BeaconVM_NullWindowSources
        NullWindow.c
    )

add_library(BeaconVMCore ${BeaconVM_Sources} ${BeaconVM_NullWindowSources})
endif()

add_executable(beacon-vm Main.c)
target_link_libraries(beacon-vm BeaconVMCore)
