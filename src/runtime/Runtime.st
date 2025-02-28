| FilesToLoad |

FilesToLoad := #(
    'ProtoObject.st'
    'Object.st'
    'Behavior.st'
    'Array.st'
    'ArrayList.st'
    'AbstractBinaryFileStream.st'
    'Number.st'
    'Slot.st'
    'SourceCode.st'
    'String.st'
    'Class.st'
    'Geometry.st'
    'Color.st'
    'Form.st'
    'Font.st'
    'FormRendering.st'
    'WindowEvents.st'
    'Window.st'
    'MorphicEvents.st'
    'Morphic.st'
    'MorphicWindow.st'
    'MorphicText.st'
    'MorphicCode.st'
    'Workspace.st'
).

1 to: FilesToLoad basicSize do: [:index |
    | fileNameToLoad filePathToLoad |
    fileNameToLoad := FilesToLoad basicAt: index.
    filePathToLoad := __FileDir__ , fileNameToLoad.

    Stdio stdout nextPutAll: 'Loading '; nextPutAll: filePathToLoad; nextPut: 10.
    filePathToLoad fileIn.
].
