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
elementAtIndex: index
    ^ nil
].

TableDataSource ![
displayStringFor: anElement
    ^ anElement printString
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
    ^  elements
].

SimpleTableDataSource ![
elements: aCollection
    elements := aCollection.
].

SimpleTableDataSource ![
elementAtIndex: index
    ^ (1 <= index and: [index <= elements size])
        ifTrue: [elements at: index]
        ifFalse: [nil]
].

BorderedMorph subclass: #TableContainerMorph instanceVariables: #().
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
computeVisibleExactRowCount
	"Answer the rows to show in list - with possible fraction"

	| visibleRows |
	^ visibleRows := self height asFloat / (self table rowHeight + self table intercellSpacing y).
].

TableContainerMorph ![
computeVisibleRowCount
	"Answer the maximal number of rows to shown in list"

	^ self computeVisibleExactRowCount ceiling asInteger
].

TableContainerMorph ![
computeStartingRowIndex
    ^ (self table rowPositionY asFloat / (self table rowHeight + self table intercellSpacing y)) floor asInteger
].

TableContainerMorph ![
computeRowIndexAtPosition: aPosition
    ^ ((aPosition y + self table rowPositionY asFloat) / (self table rowHeight + self table intercellSpacing y)) floor asInteger + 1
].

TableContainerMorph ![
renderRowsWithBuilder: renderingBuilder
    | visibleRowCount startingRowIndex positionY cellHeight selectionSet dataSource |
    visibleRowCount := self computeVisibleRowCount.
    startingRowIndex := self computeStartingRowIndex.

    positionY := self table rowPositionY.
    cellHeight := self table rowHeight + self table intercellSpacing y.

    dataSource := self table dataSource.
    dataSource == nil ifTrue: [^ self].

    selectionSet := self table selectionSet.

    1 to: visibleRowCount do: [:displayIndex |
        | rowIndex rowPositionY isSelected rowElement rowElementString rowBounds |
        rowIndex := startingRowIndex + displayIndex.
        rowPositionY := (rowIndex - 1) * cellHeight - positionY.
        rowElement := dataSource elementAtIndex: rowIndex.
        rowElement ~~ nil ifTrue: [
            rowBounds := 0@rowPositionY extent: self width @cellHeight.

            isSelected := selectionSet includes: rowIndex.
            isSelected ifTrue: [
                renderingBuilder solidRectangle: rowBounds color: Color blue
            ].

            rowElementString := dataSource displayStringFor: rowElement.
            renderingBuilder text: rowElementString rectangle: rowBounds color: Color black fontFace: Font defaultFontFace.
        ]
    ].

    "Stdio stdout nextPutAll: 'starting '; print: startingRowIndex; nextPutAll: ' rowCount '; print: visibleRowCount; lf"
].

TableContainerMorph ![
handleMorphMouseButtonDownEvent: event
    | selectionRowIndex |
    selectionRowIndex := self computeRowIndexAtPosition: event position.
    self table selectionSet: {selectionRowIndex}.

    event wasHandled: true
].

TableContainerMorph ![
handleMorphMouseWheelEvent: event
    | rowPositionY |
    rowPositionY := self table rowPositionY.
    
    self table rowPositionY: (rowPositionY + (event scrollDelta y negated * 10) max: 0).
    event wasHandled: true
].

BorderedMorph subclass: #TableMorph instanceVariables: #(dataSource container rowPositionY rowHeight intercellSpacing selectionSet onSelectionChangedBlock).

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
    rowPositionY := 0.
    selectionSet := #().
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
container
    ^ container
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
defaultIntercellSpacing
	^ 0@0
].

TableMorph ![
intercellSpacing
    ^ intercellSpacing ~~ nil
        ifTrue: [intercellSpacing]
        ifFalse: [intercellSpacing := self class defaultIntercellSpacing]
].

TableMorph ![
hasDataSource
    ^ dataSource == nil
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
resetPosition
    rowPositionY := 0.
].

TableMorph ![
rowPositionY
    ^ rowPositionY
].

TableMorph ![
rowPositionY: anInteger
    rowPositionY := anInteger.
    self changed
].

TableMorph ![
selectionSet
    ^ selectionSet
].

TableMorph ![
selectionSet: aSetOfSelectedIndices
    selectionSet := aSetOfSelectedIndices.
    onSelectionChangedBlock ~~ nil ifTrue: [onSelectionChangedBlock value: self].
    self changed
].

TableMorph ![
selectedElements
    ^ selectionSet collect: [:eachIndex | dataSource elementAtIndex: eachIndex].
].


TableMorph ![
onSelectionChangedDo: aBlock
    onSelectionChangedBlock := aBlock
].

TableMorph ![
dataSource: aDataSource
    dataSource := aDataSource.
    dataSource table: self.
    dataSource readyToBeDisplayed.
    self resetPosition.
    self refresh.
].
