#include "ImportDirectoryWindow.h"
#include "FourFlowMainWindow.h"
#include "staticFunctions.h"
#include "pqInternals.h"
#include "vtkIncludes.h"
#include <sstream>
#include <algorithm>
#include <iostream>

ImportDirectoryWindow::ImportDirectoryWindow() : internals(new Ui_ImportDirectoryWindow) { 
	this->internals->setupUi(this); 
	this->createVortex = 0;
	this->vortexInput = 0;
	this->vortexIsoVolume = 0;
	this->createVortex = false;
	this->SAX = false;
	//connect(this->internals->importSelectedFilesButton, SIGNAL(clicked()), this, SLOT(importSelectedFilesSlot()));
	connect(this->internals->importSelectedFilesButton, SIGNAL(clicked()), this, SLOT(createVortexSlot()));
	//connect(this->internals->createVortexButton, SIGNAL(clicked()), this, SLOT(createVortexSlot()));
}

void ImportDirectoryWindow::importSelectedFilesSlot() {
	QList<QListWidgetItem*> selectedItemList = this->internals->listWidget->selectedItems();
	foreach( QListWidgetItem *widgetItem, selectedItemList ) {
		if(widgetItem->text().count(".case"))
			this->caseFiles.push_back(widgetItem->text().toStdString());
		else
			this->pvdFiles.push_back(widgetItem->text().toStdString());
	}
	this->decodeCaseFiles();
	this->hide();
}

void ImportDirectoryWindow::createVortexSlot() {
	this->createVortex = true;
	importSelectedFilesSlot();
}

void ImportDirectoryWindow::decodeCaseFiles() {
	if(this->caseFiles.size()) {
		string path = *caseFiles.begin();
		this->caseFiles.erase(caseFiles.begin());
		QStringList stringList;
		stringList << QString(path.c_str());
		/************** Create Reader*****************/
		pqServer *server = pqActiveObjects::instance().activeServer();
		vtkSMReaderFactory *readerFactory = vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
		BEGIN_UNDO_SET("Create 'Reader'");
			pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
			//pqPipelineSource* reader = builder->createReader("sources", "fourFlowEnsight", stringList, server);
			pqPipelineSource* reader = builder->createReader("sources", "ensight", stringList, server);
		END_UNDO_SET();
		pqPipelineSource *source = reader;

		/***********************************************/
		QObject::connect(source, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(updatedLoadDataSlot(pqPipelineSource*)));
		if(this->createVortex) {
			if(path.find("velocity") != string::npos)
				this->vortexInput = source;
			else if(path.find("vortexprob") != string::npos)
				this->vortexIsoVolume = source;
		}
		if(path.find("Endo")==string::npos && path.find("Epi")==string::npos && path.find("RVEndo")==string::npos && path.find("RVEpi")==string::npos)
			this->endo = false;
		else
			this->endo = true;
		if(path.find("sax")!=string::npos && !this->endo)
			this->SAX = true;
		else
			this->SAX = false;
	}
	else if(this->pvdFiles.size()) {
		string path = *pvdFiles.begin();
		this->pvdFiles.erase(pvdFiles.begin());
		this->pvdPath = path;
		QStringList stringList;
		stringList << QString(path.c_str());
		/************** Create Reader*****************/
		pqServer *server = pqActiveObjects::instance().activeServer();
		vtkSMReaderFactory *readerFactory = vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
		BEGIN_UNDO_SET("Create 'Reader'");
			pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
			pqPipelineSource* reader = builder->createReader("sources", "PVDReader", stringList, server);
		END_UNDO_SET();
		pqPipelineSource *source = reader;

		/***********************************************/
		QObject::connect(source, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(updatedLoadDataSlot(pqPipelineSource*)));
	}
}

void ImportDirectoryWindow::updatedLoadDataSlot(pqPipelineSource *source)  {
	QObject::disconnect(source, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(updatedLoadDataSlot(pqPipelineSource*)));
	QObject::connect(source, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this, SLOT(representationAddedLoadDataSlot(pqPipelineSource*, pqDataRepresentation*, int)));
}

void ImportDirectoryWindow::representationAddedLoadDataSlot(pqPipelineSource *source, pqDataRepresentation *repre, int val) {
	QObject::disconnect(source, 
		SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), 
		this, 
		SLOT(representationAddedLoadDataSlot(pqPipelineSource*, pqDataRepresentation*, int)));
	QObject::connect(repre, SIGNAL(dataUpdated()), this, SLOT(updatedLoadDataPostRepresentationSlot()));
}

void ImportDirectoryWindow::loadTexture(const QString& filename, pqDataRepresentation *Representation) {
	/*vtkObject::GlobalWarningDisplayOff();
	QFileInfo finfo(filename);

	vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
	vtkSMProxy* texture = pxm->NewProxy("textures", "ImageTexture");

	texture->SetConnectionID(Representation->getProxy()->GetConnectionID());

	texture->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
	pqSMAdaptor::setElementProperty(texture->GetProperty("FileName"), filename);
	pqSMAdaptor::setEnumerationProperty(texture->GetProperty("SourceProcess"), "Client");
	texture->UpdateVTKObjects();
	pxm->RegisterProxy("textures", vtksys::SystemTools::GetFilenameName(filename.toAscii().data()).c_str(), texture);
	texture->Delete();

	vtkObject::GlobalWarningDisplayOn();*/
}

void ImportDirectoryWindow::checkAnimationStepSanity() {
	bool error=false;
	if(pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager()) {
		if(pqAnimationScene *scene = mgr->getActiveScene()) {
			QList<double> timeSteps = scene->getTimeSteps();
			for(QList<double>::iterator iter = timeSteps.begin(); iter != timeSteps.end(); iter++) {
				if((iter == timeSteps.begin()) || (iter == (timeSteps.begin()+1))) {}
				else {
					double range1 = *(iter-1)-*(iter-2);
					double range2 = *(iter)-*(iter-1);
					double difference = range1-range2;
					if(difference<0.0) difference = -difference;
					//std::cout << difference << std::endl;
					if(difference>0.001) {
						//std::cout << "error" << std::endl;
						error=true;
					}
				}
			}
		}
	}
	if (error && vtkObject::GetGlobalWarningDisplay()) {
		vtkOStreamWrapper::EndlType endl;
		vtkOStreamWrapper::UseEndl(endl);
		vtkOStrStreamWrapper vtkmsg;
		vtkmsg << "Warning: The timesteps in the loaded files differ (this can often be seen as uneven timesteps in the timeline at the bottom of the window), this may cause the application to become unstable. To fix this it is recommended that all data to be imported into FourFlow is exported at the same time. It is _strongly_ recommended not to use FourFlow with the currently loaded files.";                               \
		vtkOutputWindowDisplayErrorText(vtkmsg.str());
		vtkmsg.rdbuf()->freeze(0); 
		vtkObject::BreakOnError();
		//vtkErrorMacro("Warning: The timesteps in the loaded files differ, this may cause the application to become unstable. To fix this it is recommended that all data to be imported into FourFlow is exported at the same time.");
	}
}

void ImportDirectoryWindow::updatedLoadDataPostRepresentationSlot() {
	pqDataRepresentation *repr = (pqDataRepresentation *)sender();

	checkAnimationStepSanity();

	//cout << "ImportDirectoryWindow::updatedLoadDataPostRepresentationSlot" << endl;
	QObject::disconnect(repr, SIGNAL(dataUpdated()), this, SLOT(updatedLoadDataPostRepresentationSlot()));
	pqPipelineSource *source = repr->getInput();
	if(source->getSMName().contains(".pvd")) {
		ifstream  pvdFile(pvdPath.c_str());
		if(pvdFile.is_open()) {
			//pqSMAdaptor::setEnumerationProperty(repr->getProxy()->GetProperty("Representation"), "Volume");
			/*QList<QVariant> list = pqSMAdaptor::getEnumerationPropertyDomain(repr->getProxy()->GetProperty("Representation"));
		    foreach(QVariant v, list)
				std::cout << v.toString().toStdString() << std::endl;*/
			/*pqPipelineRepresentation* pipelineRep = qobject_cast<pqPipelineRepresentation*>(repr);
			pipelineRep->setRepresentation("Volume");
			repr->getProxy()->UpdateVTKObjects();*/
			string line;
			getline(pvdFile, line);
			getline(pvdFile, line);
			line = line.substr(4);
			line = line.substr(0, line.size()-3);
			vector<string> tokens;
			istringstream iss(line);
			copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter<vector<string> >(tokens));
			if(tokens.size() == 3) {
				QList< QVariant > originList;
				originList << QString(tokens[0].c_str()).toFloat();
				originList << QString(tokens[1].c_str()).toFloat();
				originList << QString(tokens[2].c_str()).toFloat();
				getline(pvdFile, line);
				line = line.substr(4);
				line = line.substr(0, line.size()-3);
				istringstream iss2(line);
				tokens.clear();
				copy(istream_iterator<string>(iss2), istream_iterator<string>(), back_inserter<vector<string> >(tokens));
				if(tokens.size() == 3) {
					pqSMAdaptor::setMultipleElementProperty(repr->getProxy()->GetProperty("Origin"), originList);
					QList< QVariant > orientationList;
					orientationList << QString(tokens[0].c_str()).toFloat();
					orientationList << QString(tokens[1].c_str()).toFloat();
					orientationList << QString(tokens[2].c_str()).toFloat();
					pqSMAdaptor::setMultipleElementProperty(repr->getProxy()->GetProperty("Orientation"), orientationList);
				}
			}
		}
	}
		
	ffWindow->setColorMapGreyscale(repr);
	// Here we need to differentiate between a plane and a volume, we do this by cheking if there is a texture path 
	/*source->getProxy()->UpdateProperty("TextureFilePath", 1);
	vtkSMStringVectorProperty *prop = vtkSMStringVectorProperty::SafeDownCast(source->getProxy()->GetProperty("TextureFilePath")->GetInformationProperty());
	QString path = prop->GetElement(0);
	cout << "texture path: " << path.toStdString() << endl;

	if(path.compare("") != 0) {
		loadTexture(path, repr);
		vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
		vtkSMProxy *textureProxy = pxm->GetProxy("textures", vtksys::SystemTools::GetFilenameName(path.toAscii().data()).c_str());
		pqSMAdaptor::setProxyProperty(repr->getProxy()->GetProperty("Texture"), textureProxy);
		//addTemporalTimeSnap(source);
	}
	else {
		cout << "not a plane, source: " << source << endl;
	}*/

	if(SAX) {
		pqPipelineSource *sax = addSAX(source);
		QObject::connect(sax, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this->ffWindow, SLOT(representationAddedGreyscale(pqPipelineSource*, pqDataRepresentation*, int)));
		QObject::connect(sax, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this->ffWindow, SLOT(representationAddedSurface(pqPipelineSource*, pqDataRepresentation*, int)));
		//pqPipelineSource *saxTemporal = addTemporalTimeSnap(sax);
		//QObject::connect(saxTemporal, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this, SLOT(representationAddedGreyscale(pqPipelineSource*, pqDataRepresentation*, int)));
	}
	/*if(endo) {
		addTemporalTimeSnap(source);
	}*/
	decodeCaseFiles();
	if(createVortex && vortexInput && vortexIsoVolume) {
		pqServer *server = pqActiveObjects::instance().activeServer();
		pqApplicationCore *core = pqApplicationCore::instance();
		pqObjectBuilder *builder = core->getObjectBuilder();  
		vtkSMProxyManager *pxm = vtkSMProxyManager::GetProxyManager();
		pqServerManagerModel *sm = core->getServerManagerModel();
		pqActiveObjects *activeObjects = &pqActiveObjects::instance();

		// add an iso surface
		QMap<QString, QList<pqOutputPort*> > namedInputs;
		//vtkSMProxy *filterProxy = pxm->GetPrototypeProxy("filters", "TemporalSnapToTimeStep");
		namedInputs["Input"].append(vortexIsoVolume->getOutputPort(0));
		vortexSource = builder->createFilter("filters", "CustomContour", namedInputs, server);

		namedInputs.clear();

		namedInputs["Input"].append(vortexInput->getOutputPort(0));
		namedInputs["Source"].append(vortexSource->getOutputPort(0));
		pqPipelineSource *streamTracerSource = builder->createFilter("filters", "CustomShortStreamLines", namedInputs, server);
		this->ffWindow->internals->preferedColorVariable = "AngularVelocity";
		this->ffWindow->internals->useVelocityForColor(streamTracerSource);

		createVortex = false;
		vortexInput = 0;
		vortexIsoVolume = 0;

		QObject::connect(vortexSource, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this, SLOT(representationAddedVortex(pqPipelineSource*, pqDataRepresentation*, int)));
		QObject::connect(streamTracerSource, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this, SLOT(representationAddedStreamTracer(pqPipelineSource*, pqDataRepresentation*, int)));
	}
}

void ImportDirectoryWindow::representationAddedStreamTracer(pqPipelineSource *source, pqDataRepresentation *repr, int val) {
	QObject::disconnect(source, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this, SLOT(representationAddedStreamTracer(pqPipelineSource*, pqDataRepresentation*, int)));
	pqPipelineRepresentation *pipeRep = (pqPipelineRepresentation*)repr;
}

void ImportDirectoryWindow::representationAddedVortex(pqPipelineSource *source, pqDataRepresentation *repre, int val) {
	QObject::disconnect(source, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this, SLOT(representationAddedVortex(pqPipelineSource*, pqDataRepresentation*, int)));
	pqDisplayPolicy* display_policy = pqApplicationCore::instance()->getDisplayPolicy();
	display_policy->setRepresentationVisibility(source->getOutputPort(0), pqActiveObjects::instance().activeView(), true);
}
