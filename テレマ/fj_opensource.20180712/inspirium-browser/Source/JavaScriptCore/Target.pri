# -------------------------------------------------------------------
# Target file for the JavaScriptSource library
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = lib
TARGET = JavaScriptCore

include(JavaScriptCore.pri)

WEBKIT += wtf
QT += core
QT -= gui

!win32-* {
    CONFIG += staticlib
}

*-g++*:QMAKE_CXXFLAGS_RELEASE -= -O2
*-g++*:QMAKE_CXXFLAGS_RELEASE += -O3

# Rules when JIT enabled (not disabled)
#!contains(DEFINES, ENABLE_JIT=0) {
#    *linux*-g++*:greaterThan(QT_GCC_MAJOR_VERSION,3):greaterThan(QT_GCC_MINOR_VERSION,0) {
#        QMAKE_CXXFLAGS += -fno-stack-protector
#        QMAKE_CFLAGS += -fno-stack-protector
#        QMAKE_CXXFLAGS -= -fstack-protector-all
#        QMAKE_CFLAGS -= -fstack-protector-all
#    }
#}

QMAKE_CXXFLAGS += -fno-omit-frame-pointer

include(yarr/yarr.pri)

INSTALLDEPS += all

debug_and_release: INCLUDEPATH += $$JAVASCRIPTCORE_GENERATED_SOURCES_DIR/$$targetSubDir()

SOURCES += \
    API/JSBase.cpp \
    API/JSCallbackConstructor.cpp \
    API/JSCallbackFunction.cpp \
    API/JSCallbackObject.cpp \
    API/JSClassRef.cpp \
    API/JSContextRef.cpp \
    API/JSObjectRef.cpp \
    API/JSScriptRef.cpp \
    API/JSStringRef.cpp \
    API/JSStringRefQt.cpp \
    API/JSValueRef.cpp \
    API/JSWeakObjectMapRefPrivate.cpp \
    API/OpaqueJSString.cpp \
    assembler/ARMAssembler.cpp \
    assembler/ARMv7Assembler.cpp \
    assembler/LinkBuffer.cpp \
    assembler/MacroAssembler.cpp \
    assembler/MacroAssemblerARM.cpp \
    assembler/MacroAssemblerX86Common.cpp \
    bindings/ScriptFunctionCall.cpp \
    bindings/ScriptObject.cpp \
    bindings/ScriptValue.cpp \
    bytecode/ArrayAllocationProfile.cpp \
    bytecode/ArrayProfile.cpp \
    bytecode/BytecodeBasicBlock.cpp \
    bytecode/BytecodeLivenessAnalysis.cpp \
    bytecode/CallLinkInfo.cpp \
    bytecode/CallLinkStatus.cpp \
    bytecode/CodeBlock.cpp \
    bytecode/CodeBlockHash.cpp \
    bytecode/CodeBlockJettisoningWatchpoint.cpp \
    bytecode/CodeOrigin.cpp \
    bytecode/CodeType.cpp \
    bytecode/DFGExitProfile.cpp \
    bytecode/DeferredCompilationCallback.cpp \
    bytecode/ExecutionCounter.cpp \
    bytecode/ExitKind.cpp \
    bytecode/GetByIdStatus.cpp \
    bytecode/InlineCallFrameSet.cpp \
    bytecode/JumpTable.cpp \
    bytecode/LazyOperandValueProfile.cpp \
    bytecode/MethodOfGettingAValueProfile.cpp \
    bytecode/Opcode.cpp \
    bytecode/PolymorphicPutByIdList.cpp \
    bytecode/ProfiledCodeBlockJettisoningWatchpoint.cpp \
    bytecode/PreciseJumpTargets.cpp \
    bytecode/PutByIdStatus.cpp \
    bytecode/ReduceWhitespace.cpp \
    bytecode/SamplingTool.cpp \
    bytecode/SpecialPointer.cpp \
    bytecode/SpeculatedType.cpp \
    bytecode/StructureStubClearingWatchpoint.cpp \
    bytecode/StructureStubInfo.cpp \
    bytecode/UnlinkedCodeBlock.cpp \
    bytecode/UnlinkedInstructionStream.cpp \
    bytecode/ValueRecovery.cpp \
    bytecode/Watchpoint.cpp \
    bytecompiler/BytecodeGenerator.cpp \
    bytecompiler/NodesCodegen.cpp \
    heap/CodeBlockSet.cpp \
    heap/CopiedSpaceInlines.h \
    heap/CopiedSpace.cpp \
    heap/CopyVisitor.cpp \
    heap/ConservativeRoots.cpp \
    heap/DeferGC.cpp \
    heap/Weak.cpp \
    heap/WeakBlock.cpp \
    heap/WeakHandleOwner.cpp \
    heap/WeakSet.cpp \
    heap/HandleSet.cpp \
    heap/HandleStack.cpp \
    heap/BlockAllocator.cpp \
    heap/GCThreadSharedData.cpp \
    heap/GCThread.cpp \
    heap/Heap.cpp \
    heap/HeapStatistics.cpp \
    heap/HeapTimer.cpp \
    heap/IncrementalSweeper.cpp \
    heap/JITStubRoutineSet.cpp \
    heap/MachineStackMarker.cpp \
    heap/MarkStack.cpp \
    heap/MarkedAllocator.cpp \
    heap/MarkedBlock.cpp \
    heap/MarkedSpace.cpp \
    heap/SlotVisitor.cpp \
    heap/SuperRegion.cpp \
    heap/WriteBarrierBuffer.cpp \
    heap/WriteBarrierSupport.cpp \
    debugger/DebuggerActivation.cpp \
    debugger/DebuggerCallFrame.cpp \
    debugger/Debugger.cpp \
    dfg/DFGAbstractHeap.cpp \
    dfg/DFGAbstractValue.cpp \
    dfg/DFGArgumentsSimplificationPhase.cpp \
    dfg/DFGArithMode.cpp \
    dfg/DFGArrayMode.cpp \
    dfg/DFGAtTailAbstractState.cpp \
    dfg/DFGAvailability.cpp \
    dfg/DFGBackwardsPropagationPhase.cpp \
    dfg/DFGBasicBlock.cpp \
    dfg/DFGBinarySwitch.cpp \
    dfg/DFGBlockInsertionSet.cpp \
    dfg/DFGByteCodeParser.cpp \
    dfg/DFGCapabilities.cpp \
    dfg/DFGClobberize.cpp \
    dfg/DFGClobberSet.cpp \
    dfg/DFGCommon.cpp \
    dfg/DFGCommonData.cpp \
    dfg/DFGCompilationKey.cpp \
    dfg/DFGCompilationMode.cpp \
    dfg/DFGCFAPhase.cpp \
    dfg/DFGCFGSimplificationPhase.cpp \
    dfg/DFGCPSRethreadingPhase.cpp \
    dfg/DFGConstantFoldingPhase.cpp \
    dfg/DFGCriticalEdgeBreakingPhase.cpp \
    dfg/DFGCSEPhase.cpp \
    dfg/DFGDCEPhase.cpp \
    dfg/DFGDesiredIdentifiers.cpp \
    dfg/DFGDesiredStructureChains.cpp \
    dfg/DFGDesiredTransitions.cpp \
    dfg/DFGDesiredWatchpoints.cpp \
    dfg/DFGDesiredWeakReferences.cpp \
    dfg/DFGDesiredWriteBarriers.cpp \
    dfg/DFGDisassembler.cpp \
    dfg/DFGDominators.cpp \
    dfg/DFGDriver.cpp \
    dfg/DFGEdge.cpp \
    dfg/DFGFailedFinalizer.cpp \
    dfg/DFGFinalizer.cpp \
    dfg/DFGFixupPhase.cpp \
    dfg/DFGFlushFormat.cpp \
    dfg/DFGFlushLivenessAnalysisPhase.cpp \
    dfg/DFGFlushedAt.cpp \
    dfg/DFGGraph.cpp \
    dfg/DFGInPlaceAbstractState.cpp \
    dfg/DFGInvalidationPointInjectionPhase.cpp \
    dfg/DFGJITCode.cpp \
    dfg/DFGJITCompiler.cpp \
    dfg/DFGJITFinalizer.cpp \
    dfg/DFGJumpReplacement.cpp \
    dfg/DFGLICMPhase.cpp \
    dfg/DFGLazyJSValue.cpp \
    dfg/DFGLivenessAnalysisPhase.cpp \
    dfg/DFGLongLivedState.cpp \
    dfg/DFGLoopPreHeaderCreationPhase.cpp \
    dfg/DFGMinifiedNode.cpp \
    dfg/DFGNaturalLoops.cpp \
    dfg/DFGNode.cpp \
    dfg/DFGNodeFlags.cpp \
    dfg/DFGOperations.cpp \
    dfg/DFGOSRAvailabilityAnalysisPhase.cpp \
    dfg/DFGOSREntry.cpp \
    dfg/DFGOSREntrypointCreationPhase.cpp \
    dfg/DFGOSRExit.cpp \
    dfg/DFGOSRExitBase.cpp \
    dfg/DFGOSRExitCompiler.cpp \
    dfg/DFGOSRExitCompiler64.cpp \
    dfg/DFGOSRExitCompiler32_64.cpp \
    dfg/DFGOSRExitCompilerCommon.cpp \
    dfg/DFGOSRExitJumpPlaceholder.cpp \
    dfg/DFGOSRExitPreparation.cpp \
    dfg/DFGPhase.cpp \
    dfg/DFGPlan.cpp \
    dfg/DFGPredictionPropagationPhase.cpp \
    dfg/DFGPredictionInjectionPhase.cpp \
    dfg/DFGResurrectionForValidationPhase.cpp \
    dfg/DFGSSAConversionPhase.cpp \
    dfg/DFGSSALoweringPhase.cpp \
    dfg/DFGSpeculativeJIT.cpp \
    dfg/DFGSpeculativeJIT32_64.cpp \
    dfg/DFGSpeculativeJIT64.cpp \
    dfg/DFGStackLayoutPhase.cpp \
    dfg/DFGStoreBarrierElisionPhase.cpp \
    dfg/DFGStrengthReductionPhase.cpp \
    dfg/DFGTypeCheckHoistingPhase.cpp \
    dfg/DFGThunks.cpp \
    dfg/DFGTierUpCheckInjectionPhase.cpp \
    dfg/DFGUnificationPhase.cpp \
    dfg/DFGUseKind.cpp \
    dfg/DFGValueSource.cpp \
    dfg/DFGVariableAccessDataDump.cpp \
    dfg/DFGVariableEvent.cpp \
    dfg/DFGVariableEventStream.cpp \
    dfg/DFGValidate.cpp \
    dfg/DFGVirtualRegisterAllocationPhase.cpp \
    dfg/DFGWatchpointCollectionPhase.cpp \
    dfg/DFGWorklist.cpp \
    disassembler/Disassembler.cpp \
    inspector/ConsoleMessage.cpp \
    inspector/ContentSearchUtilities.cpp \
    inspector/IdentifiersFactory.cpp \
    inspector/InjectedScript.cpp \
    inspector/InjectedScriptBase.cpp \
    inspector/InjectedScriptHost.cpp \
    inspector/InjectedScriptManager.cpp \
    inspector/InjectedScriptModule.cpp \
    inspector/InspectorAgentRegistry.cpp \
    inspector/InspectorBackendDispatcher.cpp \
    inspector/InspectorValues.cpp \
    inspector/JSInjectedScriptHost.cpp \
    inspector/JSInjectedScriptHostPrototype.cpp \
    inspector/JSJavaScriptCallFrame.cpp \
    inspector/JSJavaScriptCallFramePrototype.cpp \
    inspector/JavaScriptCallFrame.cpp \
    inspector/ScriptArguments.cpp \
    inspector/ScriptCallFrame.cpp \
    inspector/ScriptCallStack.cpp \
    inspector/ScriptCallStackFactory.cpp \
    inspector/ScriptDebugServer.cpp \
    inspector/agents/InspectorAgent.cpp \
    inspector/agents/InspectorConsoleAgent.cpp \
    inspector/agents/InspectorDebuggerAgent.cpp \
    inspector/agents/InspectorRuntimeAgent.cpp \
    interpreter/AbstractPC.cpp \
    interpreter/CallFrame.cpp \
    interpreter/Interpreter.cpp \
    interpreter/JSStack.cpp \
    interpreter/ProtoCallFrame.cpp \
    interpreter/StackVisitor.cpp \
    jit/AssemblyHelpers.cpp \
    jit/ArityCheckFailReturnThunks.cpp \
    jit/ClosureCallStubRoutine.cpp \
    jit/ExecutableAllocatorFixedVMPool.cpp \
    jit/ExecutableAllocator.cpp \
    jit/HostCallReturnValue.cpp \
    jit/GCAwareJITStubRoutine.cpp \
    jit/JITArithmetic.cpp \
    jit/JITArithmetic32_64.cpp \
    jit/JITCall.cpp \
    jit/JITCall32_64.cpp \
    jit/JITCode.cpp \
    jit/JIT.cpp \
    jit/JITDisassembler.cpp \
    jit/JITExceptions.cpp \
    jit/JITInlineCacheGenerator.cpp \
    jit/JITOpcodes.cpp \
    jit/JITOpcodes32_64.cpp \
    jit/JITOperations.cpp \
    jit/JITPropertyAccess.cpp \
    jit/JITPropertyAccess32_64.cpp \
    jit/JITStubRoutine.cpp \
    jit/JITStubs.cpp \
    jit/JITThunks.cpp \
    jit/JITToDFGDeferredCompilationCallback.cpp \
    jit/RegisterPreservationWrapperGenerator.cpp \
    jit/RegisterSet.cpp \
    jit/Repatch.cpp \
    jit/TempRegisterSet.cpp \
    jit/ThunkGenerators.cpp \
    llint/LLIntCLoop.cpp \
    llint/LLIntData.cpp \
    llint/LLIntEntrypoint.cpp \
    llint/LLIntExceptions.cpp \
    llint/LLIntSlowPaths.cpp \
    llint/LLIntThunks.cpp \
    llint/LowLevelInterpreter.cpp \
    parser/Lexer.cpp \
    parser/Nodes.cpp \
    parser/ParserArena.cpp \
    parser/Parser.cpp \
    parser/SourceCode.cpp \
    parser/SourceProvider.cpp \
    parser/SourceProviderCache.cpp \
    profiler/ProfilerBytecode.cpp \
    profiler/ProfilerBytecode.h \
    profiler/ProfilerBytecodeSequence.cpp \
    profiler/ProfilerBytecodes.cpp \
    profiler/ProfilerBytecodes.h \
    profiler/ProfilerCompilation.cpp \
    profiler/ProfilerCompilation.h \
    profiler/ProfilerCompilationKind.cpp \
    profiler/ProfilerCompilationKind.h \
    profiler/ProfilerCompiledBytecode.cpp \
    profiler/ProfilerCompiledBytecode.h \
    profiler/ProfilerDatabase.cpp \
    profiler/ProfilerDatabase.h \
    profiler/ProfilerExecutionCounter.h \
    profiler/ProfilerOrigin.cpp \
    profiler/ProfilerOrigin.h \
    profiler/ProfilerOriginStack.cpp \
    profiler/ProfilerOriginStack.h \
    profiler/ProfilerJettisonReason.cpp \
    profiler/ProfilerOSRExit.cpp \
    profiler/ProfilerOSRExitSite.cpp \
    profiler/ProfilerProfiledBytecodes.cpp \
    profiler/Profile.cpp \
    profiler/ProfileGenerator.cpp \
    profiler/ProfileNode.cpp \
    profiler/LegacyProfiler.cpp \
    runtime/ArgList.cpp \
    runtime/Arguments.cpp \
    runtime/ArgumentsIteratorConstructor.cpp \
    runtime/ArgumentsIteratorPrototype.cpp \
    runtime/ArrayBuffer.cpp \
    runtime/ArrayBufferNeuteringWatchpoint.cpp \
    runtime/ArrayBufferView.cpp \
    runtime/ArrayConstructor.cpp \
    runtime/ArrayPrototype.cpp \
    runtime/ArrayIteratorConstructor.cpp \
    runtime/ArrayIteratorPrototype.cpp \
    runtime/BooleanConstructor.cpp \
    runtime/BooleanObject.cpp \
    runtime/BooleanPrototype.cpp \
    runtime/CallData.cpp \
    runtime/CodeCache.cpp \
    runtime/CodeSpecializationKind.cpp \
    runtime/CommonIdentifiers.cpp \
    runtime/CommonSlowPaths.cpp \
    runtime/CommonSlowPathsExceptions.cpp \
    runtime/CompilationResult.cpp \
    runtime/Completion.cpp \
    runtime/ConstructData.cpp \
    runtime/DataView.cpp \
    runtime/DateConstructor.cpp \
    runtime/DateConversion.cpp \
    runtime/DateInstance.cpp \
    runtime/DatePrototype.cpp \
    runtime/DumpContext.cpp \
    runtime/Error.cpp \
    runtime/ErrorConstructor.cpp \
    runtime/ErrorHandlingScope.cpp \
    runtime/ErrorInstance.cpp \
    runtime/ErrorPrototype.cpp \
    runtime/ExceptionHelpers.cpp \
    runtime/Executable.cpp \
    runtime/FunctionConstructor.cpp \
    runtime/FunctionExecutableDump.cpp \
    runtime/FunctionPrototype.cpp \
    runtime/GCActivityCallback.cpp \
    runtime/GetterSetter.cpp \
    runtime/Identifier.cpp \
    runtime/IndexingType.cpp \
    runtime/InitializeThreading.cpp \
    runtime/IntendedStructureChain.cpp \
    runtime/InternalFunction.cpp \
    runtime/JSAPIValueWrapper.cpp \
    runtime/JSActivation.cpp \
    runtime/JSArgumentsIterator.cpp \
    runtime/JSArray.cpp \
    runtime/JSArrayBuffer.cpp \
    runtime/JSArrayBufferConstructor.cpp \
    runtime/JSArrayBufferPrototype.cpp \
    runtime/JSArrayBufferView.cpp \
    runtime/JSArrayIterator.cpp \
    runtime/JSBoundFunction.cpp \
    runtime/JSCJSValue.cpp \
    runtime/JSCell.cpp \
    runtime/JSDataView.cpp \
    runtime/JSDataViewPrototype.cpp \
    runtime/JSDateMath.cpp \
    runtime/JSFunction.cpp \
    runtime/JSGlobalObject.cpp \
    runtime/JSGlobalObjectFunctions.cpp \
    runtime/JSLock.cpp \
    runtime/JSMap.cpp \
    runtime/JSMapIterator.cpp \
    runtime/JSNameScope.cpp \
    runtime/JSNameScope.cpp \
    runtime/JSNotAnObject.cpp \
    runtime/JSONObject.cpp \
    runtime/JSObject.cpp \
    runtime/JSPromise.cpp \
    runtime/JSPromiseConstructor.cpp \
    runtime/JSPromiseDeferred.cpp \
    runtime/JSPromiseFunctions.cpp \
    runtime/JSPromiseReaction.cpp \
    runtime/JSPromisePrototype.cpp \
    runtime/JSPropertyNameIterator.cpp \
    runtime/JSProxy.cpp \
    runtime/JSSet.cpp \
    runtime/JSSetIterator.cpp \
    runtime/JSScope.cpp \
    runtime/JSSegmentedVariableObject.cpp \
    runtime/JSString.cpp \
    runtime/JSStringJoiner.cpp \
    runtime/JSSymbolTableObject.cpp \
    runtime/JSTypedArrayConstructors.cpp \
    runtime/JSTypedArrayPrototypes.cpp \
    runtime/JSTypedArrays.cpp \
    runtime/JSVariableObject.cpp \
    runtime/JSWeakMap.cpp \
    runtime/JSWithScope.cpp \
    runtime/JSWrapperObject.cpp \
    runtime/LiteralParser.cpp \
    runtime/Lookup.cpp \
    runtime/MapConstructor.cpp \
    runtime/MapData.cpp \
    runtime/MapIteratorConstructor.cpp \
    runtime/MapIteratorPrototype.cpp \
    runtime/MapPrototype.cpp \
    runtime/MathObject.cpp \
    runtime/MemoryStatistics.cpp \
    runtime/NameConstructor.cpp \
    runtime/NameInstance.cpp \
    runtime/NamePrototype.cpp \
    runtime/NativeErrorConstructor.cpp \
    runtime/NativeErrorPrototype.cpp \
    runtime/NumberConstructor.cpp \
    runtime/NumberObject.cpp \
    runtime/NumberPrototype.cpp \
    runtime/ObjectConstructor.cpp \
    runtime/ObjectPrototype.cpp \
    runtime/Operations.cpp \
    runtime/Options.cpp \
    runtime/PropertyDescriptor.cpp \
    runtime/PropertyNameArray.cpp \
    runtime/PropertySlot.cpp \
    runtime/PropertyTable.cpp \
    runtime/PrototypeMap.cpp \
    runtime/RegExp.cpp \
    runtime/RegExpCache.cpp \
    runtime/RegExpCachedResult.cpp \
    runtime/RegExpConstructor.cpp \
    runtime/RegExpMatchesArray.cpp \
    runtime/RegExpObject.cpp \
    runtime/RegExpPrototype.cpp \
    runtime/SamplingCounter.cpp \
    runtime/SetConstructor.cpp \
    runtime/SetIteratorConstructor.cpp \
    runtime/SetIteratorPrototype.cpp \
    runtime/SetPrototype.cpp \
    runtime/SimpleTypedArrayController.cpp \
    runtime/SmallStrings.cpp \
    runtime/SparseArrayValueMap.cpp \
    runtime/StrictEvalActivation.cpp \
    runtime/StringConstructor.cpp \
    runtime/StringObject.cpp \
    runtime/StringPrototype.cpp \
    runtime/StringRecursionChecker.cpp \
    runtime/Structure.cpp \
    runtime/StructureChain.cpp \
    runtime/StructureRareData.cpp \
    runtime/SymbolTable.cpp \
    runtime/TestRunnerUtils.cpp \
    runtime/TypedArrayController.cpp \
    runtime/TypedArrayType.cpp \
    runtime/VM.cpp \
    runtime/VMEntryScope.cpp \
    runtime/Watchdog.cpp \
    runtime/WatchdogQt.cpp \
    runtime/WeakMapConstructor.cpp \
    runtime/WeakMapData.cpp \
    runtime/WeakMapPrototype.cpp \
    tools/CodeProfile.cpp \
    tools/CodeProfiling.cpp \
    yarr/YarrJIT.cpp \


enable?(INSPECTOR) {
    SOURCES += $${JAVASCRIPTCORE_GENERATED_SOURCES_DIR}/InspectorJSBackendDispatchers.cpp \
               $${JAVASCRIPTCORE_GENERATED_SOURCES_DIR}/InspectorJSFrontendDispatchers.cpp \
               $${JAVASCRIPTCORE_GENERATED_SOURCES_DIR}/InspectorJSTypeBuilders.cpp
}

*linux-*:if(isEqual(QT_ARCH, "i386")|isEqual(QT_ARCH, "x86_64")) {
    SOURCES += \
        disassembler/X86Disassembler.cpp \
        disassembler/UDis86Disassembler.cpp \
        disassembler/udis86/udis86.c \
        disassembler/udis86/udis86_decode.c \
        disassembler/udis86/udis86_input.c \
        disassembler/udis86/udis86_itab_holder.c \
        disassembler/udis86/udis86_syn-att.c \
        disassembler/udis86/udis86_syn-intel.c \
        disassembler/udis86/udis86_syn.c \
}

win32:!win32-g++*:isEqual(QT_ARCH, "x86_64"):{
    asm_compiler.commands = ml64 /c
    asm_compiler.commands +=  /Fo ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
    asm_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
    asm_compiler.input = ASM_SOURCES
    asm_compiler.variable_out = OBJECTS
    asm_compiler.name = compiling[asm] ${QMAKE_FILE_IN}
    silent:asm_compiler.commands = @echo compiling[asm] ${QMAKE_FILE_IN} && $$asm_compiler.commands
    QMAKE_EXTRA_COMPILERS += asm_compiler

}

build?(qttestsupport) {
    HEADERS += API/JSCTestRunnerUtils.h
    SOURCES += API/JSCTestRunnerUtils.cpp
}

HEADERS += $$files(*.h, true)

*sh4* {
    QMAKE_CXXFLAGS += -mieee -w
    QMAKE_CFLAGS   += -mieee -w
}

*linux*-g++*:QMAKE_LFLAGS += "-Wl,-version-script=$$SOURCE_DIR/JavaScriptCore.map"

