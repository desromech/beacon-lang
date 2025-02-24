Object subclass: #Morph instanceVariables: #(bounds owner submorphs).

Morph ![
initialize
    "super initialize."
    submorphs := #().
].

Morph ![
openInWindow
    Stdio stdout nextPutAll: 'TODO: Morph >> openInWindow'; lf.
].