| FilesToLoad |

FilesToLoad := #(
    'ProtoObject.st'
    'Object.st'
    'Behavior.st'
    'Array.st'
    'ByteArray.st'
    'ArrayList.st'
    'AbstractBinaryFileStream.st'
    'Number.st'
    'Slot.st'
    'SourceCode.st'
    'Character.st'
    'Class.st'
    'CompiledCode.st'
    'Stream.st'
    'SequenceableCollection.st'
    'Printing.st'
    'String.st'
    'MethodDictionary.st'
    'Exceptions.st'
    
    'LinearAlgebra.st'

    'Geometry.st'
    'Color.st'
    'Form.st'
    'Font.st'
    'FormRendering.st'
    'WindowEvents.st'
    'Window.st'
    'MorphicEvents.st'
    'Morphic.st'
    'MorphicLayout.st'
    'MorphicWindow.st'
    'MorphicTable.st'
    'MorphicText.st'
    'MorphicCode.st'
    'Inspector.st'
    'Workspace.st'
    'ClassBrowser.st'
).

1 to: FilesToLoad basicSize do: [:index |
    | fileNameToLoad filePathToLoad |
    fileNameToLoad := FilesToLoad basicAt: index.
    filePathToLoad := __FileDir__ , fileNameToLoad.

    Stdio stdout nextPutAll: 'Loading '; nextPutAll: filePathToLoad; nextPut: 10.
    filePathToLoad fileIn.
].
