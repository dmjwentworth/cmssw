#include <map>
#include <string>
#include <cmath>
#include <vector>
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

int getTauDecayMode(const truth::Branch tauBranch) {
  // Work in progress
  int decayMode = 0; // default 
  bool printDebug = false;
  const truth::Particle tau = tauBranch.root();
  const std::vector<truth::Particle> tauDaughters = tau.children();
  const std::vector<truth::Particle> tauDescendants = tauBranch.stableLeaves();
  
	std::cout << "Children: ";
	for (truth::Particle daughter : tauDaughters) {
	  std::cout << daughter.pdgId() << " ";
	}
	std::cout << std::endl;

	std::cout << "Descendants: ";
	for (truth::Particle descendant : tauDescendants) {
	  std::cout << descendant.pdgId() << " ";
	}
	std::cout << "\n" << std::endl;

  return decayMode;
}

class TenTauAnalyser : public edm::global::EDAnalyzer<> {
public:
  explicit TenTauAnalyser(edm::ParameterSet const& cfg)
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

void TenTauAnalyser::analyze(edm::StreamID, edm::Event const& event, edm::EventSetup const&) const {
  auto const& graph = event.get(graphToken_);          // truth::Graph
  auto const& hits  = event.get(hitIndexToken_);       // truth::LogicalGraphHitIndex

  for (truth::Particle p : graph.particleViews()) { // loop over all particles
    if (!p.valid()) {
      continue; // skip invalid particles
    }
    
    if (std::abs(p.pdgId()) == 15) { // tau leptons
      const truth::Branch tauBranch(&graph, p.id());
      histograms_.at("mTauVis")->Fill(tauBranch.visibleP4().mass());
      const int decayMode = getTauDecayMode(tauBranch);
    }
  }
}

void TenTauAnalyser::beginJob() {
  // register to the TFileService
  edm::Service<TFileService> fs;
  
  // create histograms
  histograms_["mTauVis"] = fs->make<TH1F>("mTauVis", "Visible Mass of Tau Lepton", 50, 0, 200);
  histograms_["mTauVis"]->GetXaxis()->SetTitle("m_{#tau}^{vis} [GeV]");
  histograms_["mTauVis"]->GetYaxis()->SetTitle("Events");
}

void TenTauAnalyser::endJob() {
  std::cout << "Done!" << std::endl;
}

DEFINE_FWK_MODULE(TenTauAnalyser);

