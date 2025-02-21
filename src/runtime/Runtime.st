| FilesToLoad |

FilesToLoad := #(
    'Object.st'
    'Behavior.st'
    'Window.st'
).

1 to: FilesToLoad basicSize do: [:index |
    | fileNameToLoad filePathToLoad |
    fileNameToLoad := FilesToLoad basicAt: index.
    filePathToLoad := __FileDir__ , fileNameToLoad.

    Stdio stdout nextPutAll: 'Loading '; nextPutAll: filePathToLoad; nextPut: 10.
    filePathToLoad fileIn.
].
