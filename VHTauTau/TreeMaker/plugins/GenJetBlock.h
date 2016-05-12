#ifndef __VHTauTau_TreeMaker_GenJetBlock_h
#define __VHTauTau_TreeMaker_GenJetBlock_h

#include <string>
#include <vector>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "VHTauTau/TreeMaker/interface/PhysicsObjects.h"

class TClonesArray;
class GenJet;

class GenJetBlock : public edm::EDAnalyzer 
{
private:
  virtual void beginJob();
  virtual void beginRun(edm::Run const& iRun, edm::EventSetup const& iSetup) {}
  virtual void analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup);
  virtual void endJob() {}

public:
  explicit GenJetBlock(const edm::ParameterSet& iConfig);
  virtual ~GenJetBlock() {}

  enum {
    kMaxGenJet = 100
  };

private:
  TClonesArray* cloneGenJet; 
  int  fnGenJet;
  int _verbosity;
  edm::InputTag _inputTag;

  vhtm::GenJet* genJetB; 
};
#endif
