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
    'Integer.st'
    'Slot.st'
    'SourceCode.st'
    'Character.st'
    'Class.st'
    'CompiledCode.st'
    'Stream.st'
    'Printing.st'
    'SequenceableCollection.st'
    'String.st'
    'MethodDictionary.st'
    'Exceptions.st'
    'Directory.st'
    'File.st'

    'TestCase.st'
    
    'LinearAlgebra.st'
    'ReversibleAffineTransform3D.st'
    'TRSTransform3D.st'

    'Geometry.st'
    'Geometry3D.st'
    'Color.st'
    'Form.st'
    'Font.st'
    'FormRendering.st'
    'Rendering3D.st'
    'MeshBuilder.st'
    'WindowEvents.st'
    'Window.st'
    'MorphicEvents.st'
    'Morphic.st'
    'MorphicLayout.st'
    'MorphicWindow.st'
    'MorphicTable.st'
    'MorphicText.st'
    'MorphicToolBar.st'
    'MorphicCode.st'
    'MorphicMenuBar.st'
    'MorphicStatusBar.st'
    'MorphicCube.st'
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
