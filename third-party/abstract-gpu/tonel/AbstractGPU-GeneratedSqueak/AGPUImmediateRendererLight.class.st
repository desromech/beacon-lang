Class {
	#name : #AGPUImmediateRendererLight,
	#pools : [
		'AGPUConstants'
	],
	#superclass : #ExternalUnion,
	#category : 'AbstractGPU-GeneratedSqueak'
}

{ #category : #'definition' }
AGPUImmediateRendererLight class >> fields [
	"
	self defineFields
	"
    ^ #(
		(classic 'AGPUImmediateRendererLightClassic')
		(pbr 'AGPUImmediateRendererLightPbr')
	)
]

