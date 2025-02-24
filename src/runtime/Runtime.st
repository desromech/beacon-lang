| FilesToLoad |

FilesToLoad := #(
    'Object.st'
    'Behavior.st'
    'Slot.st'
    'Class.st'
    'AbstractBinaryFileStream.st'
    'Geometry.st'
    'Color.st'
    'Form.st'
    'FormRendering.st'
    'WindowEvents.st'
    'Window.st'
    'MorphicEvents.st'
    'Morphic.st'
    'MorphicWindow.st'
).

1 to: FilesToLoad basicSize do: [:index |
    | fileNameToLoad filePathToLoad |
    fileNameToLoad := FilesToLoad basicAt: index.
    filePathToLoad := __FileDir__ , fileNameToLoad.

    Stdio stdout nextPutAll: 'Loading '; nextPutAll: filePathToLoad; nextPut: 10.
    filePathToLoad fileIn.
].
