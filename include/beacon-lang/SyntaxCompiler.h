#ifndef BEACON_LANGUAGE_SYNTAX_COMPILER_H
#define BEACON_LANGUAGE_SYNTAX_COMPILER_H

#pragma once

#include "ObjectModel.h"
#include "Parser.h"
#include "Bytecode.h"

beacon_CompiledMethod_t *beacon_compileReplSyntax(beacon_context_t *context, beacon_ParseTreeNode_t *parseTree);

#endif //BEACON_LANGUAGE_SYNTAX_COMPILER_H
