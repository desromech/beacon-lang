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

add_library(BeaconVMCore ${BeaconVM_Sources})

add_executable(beacon-vm Main.c)
target_link_libraries(beacon-vm BeaconVMCore)
