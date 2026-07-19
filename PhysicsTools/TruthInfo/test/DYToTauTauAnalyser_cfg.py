import FWCore.ParameterSet.Config as cms

process = cms.Process("DYToTauTauAnalyser")

# Messages
process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("Validation.Configuration.truthPrevalidation_cff")

process.maxEvents = cms.untracked.PSet(
    input=cms.untracked.int32(-1)
)

process.source = cms.Source(
    "PoolSource",
    fileNames=cms.untracked.vstring(
        "file:/eos/user/d/dwentwor/truth-graph/output/samples/34045.88_DYtoTauTau_M_50_14TeV+Run4D120_enableTruth/step3.root"
    )
)

process.TFileService = cms.Service(
    "TFileService",
    fileName = cms.string("/eos/user/d/dwentwor/truth-graph/output/task_1_plots/DYToTauTau.root")
)

process.truthGraphAnalyser = cms.EDAnalyzer(
    "DYToTauTauAnalyser",
    src = cms.InputTag("truthLogicalGraphProducer"),
    hitIndex = cms.InputTag("truthLogicalGraphHitIndexProducer"),
)

process.truthanalyzer = cms.Path(
    process.truthGraphPrevalidation
    + process.truthGraphAnalyser
)

