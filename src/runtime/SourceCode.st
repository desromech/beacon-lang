SourceCode ![
directory
    ^ directory
].

SourceCode ![
directory: aDirectory
    directory := aDirectory
].

SourceCode ![
name
    ^ name
].

SourceCode ![
name: aName
    name := aName
].

SourceCode ![
text
    ^ text
].

SourceCode ![
text: aText
    text := aText
].

SourceCode ![
textSize
    ^ textSize
].

SourceCode ![
textSize: aTextSize
    textSize := aTextSize
].

SourceCode ![
compileAndRunIt
    | scannedSource parsedSource |
    scannedSource := self scan.
    parsedSource := self parseScannedSource: scannedSource.
    ^ self evaluateFileSyntaxWithParsedCode: parsedSource
].