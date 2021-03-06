#ifndef psInternals_h
#define psInternals_h

#include "ui_MainWindow.h"
#include "qtIncludes.h"
#include "pqDataRepresentation.h"

class FourFlowMainWindow;

class pqInternals : public QObject, public Ui::MainWindow {
	Q_OBJECT
	public:
		FourFlowMainWindow *fourFlowMainWindow;
		std::string preferedColorVariable;
		pqInternals();
		
		void setUpDefaults(FourFlowMainWindow *mainWindow);
		void addToolbars(FourFlowMainWindow *mainWindow);
		void setUpAdvancedMenu();
		void connectButtonSignalsWithSlots();
		void useVelocityForColor(pqPipelineSource *source);
		void useColorVariable(pqDataRepresentation *repr);
	protected slots:
		void openAdvancedMenu();
		void polygonEditorFrameClosed();
		void polygonEditorToggle();
		void applyProbePlane();
		void MyNewprobeplane();
	    void showHelpForProxy(const QString& proxyname);
		void applyParticleTrace();
		void applyStreamlines();
		void applyGraph();
		void applyVolume();
		void applyPathlines();
		void applyParticleCollector();
		void applyVolumeTrace();
		void applyIsoSurface();
		void applyClip();
		void updatedColor();
		void representationAddedColor(pqPipelineSource *source, pqDataRepresentation *repr, int val);
		void onRepresentationAddedVolume(pqPipelineSource *source, pqDataRepresentation *repr, int srcOutputPort);
		void dataUpdatedVolume();
		void onUndo();
		void onRedo();
		void onRepresentationAddedProbePlane(pqPipelineSource*, pqDataRepresentation *rep, int);
		void probePlaneUpdated();
		void updateEnableState();
	private:
		QMenu advancedMenu;
		bool animationHidden, propertiesHidden, polygonEditorHidden;
		pqView *polygonEditorView;

		pqPipelineSource *autoConnectFilter(std::string group, std::string filter, bool color);
};

#endif