#include <map>
#include <string>
#include <cmath>
#include <vector>
#include <span>
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
#include "PhysicsTools/TruthInfo/interface/BranchHitAssociator.h"
#include "SimDataFormats/TruthInfo/interface/LogicalGraphHitIndex.h"

class ZMMRecAnalyser : public edm::global::EDAnalyzer<> {
public:
  explicit ZMMRecAnalyser(edm::ParameterSet const& cfg)
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

void ZMMRecAnalyser::analyze(edm::StreamID, edm::Event const& event, edm::EventSetup const&) const {
  auto const& graph = event.get(graphToken_);          // truth::Graph
  auto const& hits  = event.get(hitIndexToken_);       // truth::LogicalGraphHitIndex

  for (truth::Particle p : graph.particleViews()) { // loop over all particles
    if (!p.valid()) {
      continue; // skip invalid particles
    }

    if (p.pdgId() == 23 && p.hasGen()) { // Z boson at generator level
      const std::vector<truth::Particle> bosonDaughters = p.children();
      if (bosonDaughters.size() == 2) {
        const truth::Particle muon1 = bosonDaughters[0];
        const truth::Particle muon2 = bosonDaughters[1];
        math::XYZTLorentzVectorD genP4 = muon1.momentum() + muon2.momentum();

        // filling histograms for generator level Z mass
        if (muon1.pdgId() == 13 && muon2.pdgId() == -13) { // muon1 is mu- and muon2 is mu+
          histograms_.at("GenZMass")->Fill(genP4.mass());
          histograms_.at("GenMu1Pt")->Fill(muon1.momentum().pt());
          histograms_.at("GenMu2Pt")->Fill(muon2.momentum().pt());
          histograms_.at("GenMu1Eta")->Fill(muon1.momentum().eta());
          histograms_.at("GenMu2Eta")->Fill(muon2.momentum().eta());
        }
        else if (muon1.pdgId() == -13 && muon2.pdgId() == 13) { // muon1 is mu+ and muon2 is mu-
          histograms_.at("GenZMass")->Fill(genP4.mass());
          histograms_.at("GenMu1Pt")->Fill(muon2.momentum().pt());
          histograms_.at("GenMu2Pt")->Fill(muon1.momentum().pt());
          histograms_.at("GenMu1Eta")->Fill(muon2.momentum().eta());
          histograms_.at("GenMu2Eta")->Fill(muon1.momentum().eta());
        }
        // matching reco muons to gen muons and filling histograms for reconstructed Z mass
        // const std::vector<uint32_t> candidateRoots = {muon1.id(), muon2.id()};
        // truth::BranchHitAssociator muonAssociator(hits,
        // 																				  candidateRoots,
        // 																	 				truth::Metric::SharedHits,
        // 																	 				truth::HitChannel::Muon);
        
        // for (uint32_t pid = 0; pid < hits.nParticles(); ++pid) {
        // 	std::span<const truth::LogicalGraphHitIndex::Hit> muonHits =
        // 		hits.subgraphHits(truth::HitChannel::Muon, pid); 	
        // }
      }
    }
  }  
}

void ZMMRecAnalyser::beginJob() {
  // register to the TFileService
  edm::Service<TFileService> fs;
  
  // create histograms
  histograms_["GenZMass"] = fs->make<TH1F>("GenBosonMass", "Generator level m_{#mu#mu}", 50, 60, 120);
  histograms_["GenZMass"]->GetXaxis()->SetTitle("m_{#mu#mu} [GeV]");
  histograms_["GenZMass"]->GetYaxis()->SetTitle("Events");

  histograms_["GenMu1Pt"] = fs->make<TH1F>("GenMuon1Pt", "Generator level p_{T} of #mu^{-}", 40, 0, 150);
  histograms_["GenMu1Pt"]->GetXaxis()->SetTitle("p_{T} [GeV]");
  histograms_["GenMu1Pt"]->GetYaxis()->SetTitle("Events");

  histograms_["GenMu2Pt"] = fs->make<TH1F>("GenMuon2Pt", "Generator level p_{T} of #mu^{+}", 40, 0, 150);
  histograms_["GenMu2Pt"]->GetXaxis()->SetTitle("p_{T} [GeV]");
  histograms_["GenMu2Pt"]->GetYaxis()->SetTitle("Events");

  histograms_["GenMu1Eta"] = fs->make<TH1F>("GenMuon1Eta", "Generator level #eta of #mu^{-}", 20, -4, 4);
  histograms_["GenMu1Eta"]->GetXaxis()->SetTitle("#eta");
  histograms_["GenMu1Eta"]->GetYaxis()->SetTitle("Events");

  histograms_["GenMu2Eta"] = fs->make<TH1F>("GenMuon2Eta", "Generator level #eta of #mu^{+}", 20, -4, 4);
  histograms_["GenMu2Eta"]->GetXaxis()->SetTitle("#eta");
  histograms_["GenMu2Eta"]->GetYaxis()->SetTitle("Events");

	// histograms_["RecZMass"] = fs->make<TH1F>("RecBosonMass", "Reconstructed m_{#mu#mu}", 50, 60, 120);
	// histograms_["RecZMass"]->GetXaxis()->SetTitle("m_{#mu#mu} [GeV]");
	// histograms_["RecZMass"]->GetYaxis()->SetTitle("Events");
}

void ZMMRecAnalyser::endJob() {
  std::cout << "Done!" << std::endl;
}

DEFINE_FWK_MODULE(ZMMRecAnalyser);

