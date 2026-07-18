#include <map>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>
#include "TH1.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Framework/interface/global/EDAnalyzer.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/Math/interface/LorentzVector.h"
#include "SimDataFormats/TruthInfo/interface/Graph.h"
#include "PhysicsTools/TruthInfo/interface/Branch.h"
#include "SimDataFormats/TruthInfo/interface/LogicalGraphHitIndex.h"

class DYToTauTauAnalyser : public edm::global::EDAnalyzer<> {
public:
  explicit DYToTauTauAnalyser(edm::ParameterSet const& cfg)
    : graphToken_(consumes<truth::Graph>(cfg.getParameter<edm::InputTag>("src"))),
      hitIndexToken_(consumes<truth::LogicalGraphHitIndex>(cfg.getParameter<edm::InputTag>("hitIndex"))) {}

private:
  void analyze(edm::StreamID, edm::Event const& event, edm::EventSetup const&) const override;
  void beginJob() override;
  void endJob() override;

  const edm::EDGetTokenT<truth::Graph> graphToken_;
  const edm::EDGetTokenT<truth::LogicalGraphHitIndex> hitIndexToken_;

  std::map<std::string, TH1F*> histograms_;
};

void DYToTauTauAnalyser::analyze(edm::StreamID, edm::Event const& event, edm::EventSetup const&) const {
  auto const& graph = event.get(graphToken_);          // truth::Graph
  auto const& hits  = event.get(hitIndexToken_);       // truth::LogicalGraphHitIndex

  for (truth::Particle p : graph.particleViews()) { // loop over all particles
    if (!p.valid()) {
      continue; // skip invalid particles
    }
    
    if (p.pdgId() == 23 && p.hasGen()) { // Z boson at generator level
      const truth::Branch bosonBranch(&graph, p.id());
      
      if (bosonBranch.isSignal()) {
        histograms_.at("GenZMass")->Fill(p.momentum().mass());
        histograms_.at("mTauTauVis")->Fill(bosonBranch.visibleP4().mass());
        math::XYZTLorentzVectorD pMiss = bosonBranch.p4() - bosonBranch.visibleP4();
        histograms_.at("pTmiss")->Fill(pMiss.pt());

        std::vector<truth::Particle> bosonDaughters = p.children();
        if (bosonDaughters.size() == 2) {
          for (truth::Particle tau : bosonDaughters) {
            if (abs(tau.pdgId()) == 15) { // tau lepton
              const truth::Branch tauBranch(&graph, tau.id());
            }
          }
        }
      }
    }
  }
}

void DYToTauTauAnalyser::beginJob() {
  // register to the TFileService
  edm::Service<TFileService> fs;
  
  // create histograms
  histograms_["GenZMass"] = fs->make<TH1F>("GenBosonMass", "m_{Z}", 50, 60, 120);
  histograms_["GenZMass"]->GetXaxis()->SetTitle("m_{Z} [GeV]");
  histograms_["GenZMass"]->GetYaxis()->SetTitle("Events");

  histograms_["mTauTauVis"] = fs->make<TH1F>("mTauTauVis", "m_{#tau#tau}^{vis}", 50, 0, 300);
  histograms_["mTauTauVis"]->GetXaxis()->SetTitle("m_{#tau#tau}^{vis} [GeV]");
  histograms_["mTauTauVis"]->GetYaxis()->SetTitle("Events");

  histograms_["pTmiss"] = fs->make<TH1F>("pTmiss", "p_{T}^{miss}", 50, 0, 75);
  histograms_["pTmiss"]->GetXaxis()->SetTitle("p_{T}^{miss} [GeV]");
  histograms_["pTmiss"]->GetYaxis()->SetTitle("Events");

  histograms_["GenDecayMode"] = fs->make<TH1F>("GenDecayMode", "GenDecayMode", 3, 0, 3);
  histograms_["GenDecayMode"]->GetXaxis()->SetTitle("GenDecayMode");
  histograms_["GenDecayMode"]->GetYaxis()->SetTitle("Events");
}

void DYToTauTauAnalyser::endJob() {
  std::cout << "Done!" << std::endl;
}

DEFINE_FWK_MODULE(DYToTauTauAnalyser);

std::string getDecayMode(const truth::Branch& tauBranch) {
  
  return decayMode;
}
