Object subclass: #TableDataSource instanceVariables: #(table).

TableDataSource ![
table
    ^ table
].

TableDataSource ![
table: aTable
    ^ table
].

Morph subclass: #TableMorph instanceVariables: #().

TableMorph class ![
defaultExtent
    ^ 400@300.
].

TableMorph ![
refresh
    self changed
].

TableMorph class ![
defaultColor
    ^ Color white.
].

"TableMorph ![
renderWithBuilder: renderingBuilder
    renderingBuilder solidRectangle: self localBounds color: color
]."