Object subclass: #TableDataSource instanceVariables: #(table).

TableDataSource ![
table
    ^ table
].

TableDataSource ![
table: aTable
    ^ table
].

TableDataSource ![
readyToBeDisplayed

].


TableDataSource subclass: #SimpleTableDataSource instanceVariables: #(elements).

SimpleTableDataSource class ![
elements: aCollection
    ^ self new elements: aCollection; yourself
].

SimpleTableDataSource ![
elements
    ^  elements.
].

SimpleTableDataSource ![
elements: aCollection
    elements := aCollection.
].

Morph subclass: #TableContainerMorph instanceVariables: #(headerRow exposedRows).
TableContainerMorph class ![
defaultColor
    ^ Color white
].

TableContainerMorph ![
table
    ^ self owner
].

TableContainerMorph ![
renderWithBuilder: renderingBuilder
    super renderWithBuilder: renderingBuilder.
    self renderRowsWithBuilder: renderingBuilder
].

TableContainerMorph ![
exposedRows
    ^ exposedRows
].

TableContainerMorph ![
calculateExactVisibleRows
	"Answer the rows to show in list - with possible fraction"

	| visibleRows |
	visibleRows := self height asFloat / (self table rowHeight + self table intercellSpacing y).
	^ headerRow ~~ nil 
		ifTrue: [ visibleRows - 1 ]
		ifFalse: [ visibleRows ]
].

TableContainerMorph ![
calculateMaxVisibleRows
	"Answer the maximal number of rows to shown in list"

	^ self calculateExactVisibleRows ceiling asInteger
].

TableContainerMorph ![
calculateStartIndexWhenShowing: visibleRows
	"Answer the first row to show when showing visibleRows rows.
	 This works in case we are exceeding the available rows to show"
	| currentIndex startIndex oldIndex |

	currentIndex := self table showIndex.
	currentIndex + visibleRows - 1 > self table numberOfRows
		ifTrue: [ currentIndex := self table numberOfRows - visibleRows + 2 ].
	startIndex := currentIndex max: 1.
	oldIndex := self table showIndex.
	"self table basicMoveShowIndexTo: startIndex.
	self table announceScrollChangedFrom: oldIndex to: self table showIndex."
	^ startIndex
].

TableContainerMorph ![
calculateColumnWidths
    | widths |
    widths := Array new: self table numberOfColumns.
    1 to: widths size do: [:i |
        widths at: i put: 0
    ].

    ^ widths
].

TableContainerMorph ![
updateExposedRows
    | visibleRows columns columnWidths startIndex |

    visibleRows := self calculateMaxVisibleRows.
	startIndex := self calculateStartIndexWhenShowing: visibleRows.
	columns := self table columns.
	columnWidths := self calculateColumnWidths.

    exposedRows := MethodDictionary new.
].

TableContainerMorph ![
renderRowsWithBuilder: renderingBuilder
    | x y cellWidth cellHeight |

].

BorderedMorph subclass: #TableMorph instanceVariables: #(dataSource showIndex columns container rowHeight intercellSpacing).

TableMorph class ![
defaultExtent
    ^ 400@300.
].

TableMorph class ![
defaultColor
    ^ Color r: 0.8 g: 0.8 b: 0.8 a: 1.0.
].

TableMorph ![
initialize
    super initialize.
    showIndex := 0.
    columns := #().
    self initializeContainer.

    self layout: (HorizontalMorphPackingLayout new
        addMorph: container proportion: 1;
        yourself)

].

TableMorph ![
defaultContainer
    ^ TableContainerMorph new
].

TableMorph ![
initializeContainer
    container := self defaultContainer.
    self addMorph: container
].

TableMorph ![
columns
    ^ columns
].

TableMorph ![
columns: aCollection
    columns := aCollection asArray
].

TableMorph ![
container
    ^ container
].

TableMorph ![
resetPosition
    showIndex := 0.
    self container updateExposedRows
].

TableMorph ![
refresh
    self changed
].

TableMorph class ![
defaultRowHeight
	^ Font defaultFontFace height + 7
].

TableMorph ![
rowHeight
    ^ rowHeight ~~ nil
        ifTrue: [rowHeight]
        ifFalse: [rowHeight := self class defaultRowHeight]
].

TableMorph class ![
defaultIntercelSpacing
	^ 0@0
].

TableMorph ![
intercellSpacing
    ^ intercellSpacing ~~ nil
        ifTrue: [intercellSpacing]
        ifFalse: [intercellSpacing := self class defaultIntercelSpacing]
].

TableMorph ![
showIndex
    ^ showIndex
].

TableMorph ![
hasDataSource
    ^ dataSource == nil
].

TableMorph ![
numberOfColumns
	^ columns size
].

TableMorph ![
numberOfRows
    self hasDataSource ifFalse: [ ^ 0 ].
    ^ dataSource numberOfRows
].

TableMorph ![
dataSource
    ^ dataSource
].

TableMorph ![
dataSource: aDataSource
    dataSource := aDataSource.
    dataSource table: self.
    dataSource readyToBeDisplayed.
    self resetPosition.
    self refresh.
].
