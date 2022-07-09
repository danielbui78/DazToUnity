#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QToolTip>
#include <QtGui/QWhatsThis>
#include <QtGui/qlineedit.h>
#include <QtGui/qboxlayout.h>
#include <QtGui/qfiledialog.h>
#include <QtCore/qsettings.h>
#include <QtGui/qformlayout.h>
#include <QtGui/qcombobox.h>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qcheckbox.h>
#include <QtGui/qlistwidget.h>
#include <QtGui/qgroupbox.h>

#include "dzapp.h"
#include "dzscene.h"
#include "dzstyle.h"
#include "dzmainwindow.h"
#include "dzactionmgr.h"
#include "dzaction.h"
#include "dzskeleton.h"
#include "qstandarditemmodel.h"

#include "DzUnityDialog.h"
#include "DzBridgeAction.h"
#include "DzBridgeMorphSelectionDialog.h"
#include "DzBridgeSubdivisionDialog.h"

#include "version.h"

#ifdef WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

/*****************************
Local definitions
*****************************/
#define DAZ_TO_UNITY_PLUGIN_NAME "DazToUnity"

#include "dzbridge.h"

DzUnityDialog::DzUnityDialog(QWidget* parent) :
	 DzBridgeDialog(parent, DAZ_TO_UNITY_PLUGIN_NAME)
{
	 projectEdit = nullptr;
	 projectButton = nullptr;
	 assetsFolderEdit = nullptr;
	 assetsFolderButton = nullptr;
	 installUnityFilesCheckBox = nullptr;

	 settings = new QSettings("Daz 3D", "DazToUnity");

	 // Declarations
	 int margin = style()->pixelMetric(DZ_PM_GeneralMargin);
	 int wgtHeight = style()->pixelMetric(DZ_PM_ButtonHeight);
	 int btnMinWidth = style()->pixelMetric(DZ_PM_ButtonMinWidth);

	 // Set the dialog title
#ifdef _PRE_RELEASE
	 setWindowTitle(tr("DazToUnity Bridge v%1.%2 Pre-Release Build %3.%4").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(PLUGIN_REV).arg(PLUGIN_BUILD));
#else
	 setWindowTitle(tr("DazToUnity Bridge v%1.%2").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR));
#endif

	 // Welcome String for Setup/Welcome Mode
	 QString sSetupModeString = tr("<h4>\
If this is your first time using the Daz To Unity Bridge, please be sure to read or watch \
the tutorials or videos below to install and enable the Unity Plugin for the bridge:</h4>\
<ul>\
<li><a href=\"https://github.com/daz3d/DazToUnity/releases\">Download latest Build dependencies, updates and bugfixes (Github)</a></li>\
<li><a href=\"https://github.com/daz3d/DazToUnity#2-how-to-install\">How To Install and Configure the Bridge (Github)</a></li>\
<li><a href=\"https://www.daz3d.com/unity-bridge#faq\">Daz To Unity FAQ (Daz 3D)</a></li>\
<li><a href=\"https://www.daz3d.com/forums/discussion/573571/daztounity-2022-bridge-what-s-new-and-how-to-use-it\">What's New and How To Use It (Daz 3D Forums)</a></li>\
</ul>\
Once the Unity plugin is enabled, please add a Character or Prop to the Scene to transfer assets using the Daz To Unity Bridge.<br><br>\
To find out more about Daz Bridges, go to <a href=\"https://www.daz3d.com/daz-bridges\">https://www.daz3d.com/daz-bridges</a><br>\
");
	 m_WelcomeLabel->setText(sSetupModeString);
	 QString sBridgeVersionString = tr("Daz To Unity Bridge %1.%2 revision %3.%4").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(PLUGIN_REV).arg(PLUGIN_BUILD);
	 setBridgeVersionStringAndLabel(sBridgeVersionString);


	 // Disable unsupported AssetType ComboBox Options
	 QStandardItemModel* model = qobject_cast<QStandardItemModel*>(assetTypeCombo->model());
	 QStandardItem* item = nullptr;
	 item = model->findItems("Environment").first();
	 if (item) item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
	 item = model->findItems("Pose").first();
	 if (item) item->setFlags(item->flags() & ~Qt::ItemIsEnabled);

	 // Connect new asset type handler
	 connect(assetTypeCombo, SIGNAL(activated(int)), this, SLOT(HandleAssetTypeComboChange(int)));

	 // Intermediate Folder
	 QHBoxLayout* assetsFolderLayout = new QHBoxLayout();
	 assetsFolderEdit = new QLineEdit(this);
	 assetsFolderButton = new QPushButton("...", this);
	 assetsFolderLayout->addWidget(assetsFolderEdit);
	 assetsFolderLayout->addWidget(assetsFolderButton);
	 connect(assetsFolderEdit, SIGNAL(textChanged(const QString&)), this, SLOT(HandleAssetFolderChanged(const QString&)));
	 connect(assetsFolderButton, SIGNAL(released()), this, SLOT(HandleSelectAssetsFolderButton()));

	 // Advanced Options
	 installOrOverwriteUnityFilesLabel = new QLabel(tr("Install Unity Files"));
	 installUnityFilesCheckBox = new QCheckBox("", this);
	 connect(installUnityFilesCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleInstallUnityFilesCheckBoxChange(int)));

	 // Add the widget to the basic dialog
	 mainLayout->insertRow(1, "Unity Assets Folder", assetsFolderLayout);
	 mainLayout->insertRow(2, installOrOverwriteUnityFilesLabel, installUnityFilesCheckBox);

	 // Rename Open Intermediate Folder button
	 m_OpenIntermediateFolderButton->setText(tr("Open Unity Project Folder"));

	 // Configure Target Plugin Installer
	 renameTargetPluginInstaller("Unity Plugin Installer");
	 m_TargetSoftwareVersionCombo->clear();
	 m_TargetSoftwareVersionCombo->addItem("Select Unity Rendering Pipeline");
	 m_TargetSoftwareVersionCombo->addItem("HDRP (High-Definition Rendering Pipeline)");
	 m_TargetSoftwareVersionCombo->addItem("URP (Universal Rendering Pipeline)");
	 m_TargetSoftwareVersionCombo->addItem("Built-In (Standard Shader)");
	 showTargetPluginInstaller(true);

	 // Make the dialog fit its contents, with a minimum width, and lock it down
	 resize(QSize(500, 0).expandedTo(minimumSizeHint()));
	 setFixedWidth(width());
	 setFixedHeight(height());

	 update();

	 // Help
	 assetNameEdit->setWhatsThis("This is the name the asset will use in Unity.");
	 assetTypeCombo->setWhatsThis("Skeletal Mesh for something with moving parts, like a character\nStatic Mesh for things like props\nAnimation for a character animation.");
	 assetsFolderEdit->setWhatsThis("Unity Assets folder. DazStudio assets will be exported into a subfolder inside this folder.");
	 assetsFolderButton->setWhatsThis("Unity Assets folder. DazStudio assets will be exported into a subfolder inside this folder.");
	 m_wTargetPluginInstaller->setWhatsThis("You can install the Unity Plugin by selecting the desired Render Pipeline and then selecting a Unity Project folder.");

	 // Set Defaults
	 resetToDefaults();

	 // Load Settings
	 loadSavedSettings();

	 if (m_bSetupMode)
	 {
		 setDisabled(true);
	 }
}

bool DzUnityDialog::loadSavedSettings()
{
	DzBridgeDialog::loadSavedSettings();

	if (!settings->value("AssetsPath").isNull())
	{
		// DB (2021-05-15): check AssetsPath folder and set InstallUnityFiles if Daz3D subfolder does not exist
		QString directoryName = settings->value("AssetsPath").toString();
		assetsFolderEdit->setText(directoryName);
	}
	else
	{
		QString DefaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToUnity";
		assetsFolderEdit->setText(DefaultPath);
	}

	return true;
}

void DzUnityDialog::resetToDefaults()
{
	m_bDontSaveSettings = true;

	DzBridgeDialog::resetToDefaults();

	QString DefaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToUnity";
	assetsFolderEdit->setText(DefaultPath);

	DzNode* Selection = dzScene->getPrimarySelection();
	if (dzScene->getFilename().length() > 0)
	{
		QFileInfo fileInfo = QFileInfo(dzScene->getFilename());
		assetNameEdit->setText(fileInfo.baseName().remove(QRegExp("[^A-Za-z0-9_]")));
	}
	else if (dzScene->getPrimarySelection())
	{
		assetNameEdit->setText(Selection->getLabel().remove(QRegExp("[^A-Za-z0-9_]")));
	}

	if (qobject_cast<DzSkeleton*>(Selection))
	{
		assetTypeCombo->setCurrentIndex(0);
	}
	else
	{
		assetTypeCombo->setCurrentIndex(1);
	}

	m_bDontSaveSettings = false;
}

void DzUnityDialog::HandleAssetFolderChanged(const QString& directoryName)
{
	// DB (2021-05-15): Check for presence of Daz3D folder, and set installUnityFiles if not present
	if (QDir(directoryName + QDir::separator() + "Daz3D").exists())
	{
		// deselect install unity files
		settings->setValue("InstallUnityFiles", false);
		installUnityFilesCheckBox->setChecked(false);
		// rename label to show "Overwrite"
		installOrOverwriteUnityFilesLabel->setText(tr("Overwrite Unity Files"));
	}
	else
	{
		settings->setValue("InstallUnityFiles", true);
		installUnityFilesCheckBox->setChecked(true);
		// rename label to show "Install"
		installOrOverwriteUnityFilesLabel->setText(tr("Install Unity Files"));
	}

}

void DzUnityDialog::HandleSelectAssetsFolderButton()
{
	 // DB (2021-05-15): prepopulate with existing folder string
	 QString directoryName = "/home";
	 if (!settings->value("AssetsPath").isNull())
	 {
		 directoryName = settings->value("AssetsPath").toString();
	 }
	 directoryName = QFileDialog::getExistingDirectory(this, tr("Choose Directory"),
		  directoryName,
		  QFileDialog::ShowDirsOnly
		  | QFileDialog::DontResolveSymlinks);

	 if (directoryName != NULL)
	 {
		  // sanity check to avoid crashes
		  if (QDir(directoryName).exists() == false)
		  {
			  QMessageBox::warning(0, tr("Error"), tr("Please select Unity Root Assets Folder."), QMessageBox::Ok);
			  return;
		  }
		  if (QRegExp(".*/assets$").exactMatch(directoryName.toLower()) == false)
		  {
			  if (IsValidProjectFolder(directoryName))
			  {
				  directoryName += "/Assets";
			  }			  
		  }

		  QDir parentDir = QFileInfo(directoryName).dir();
		  if (IsValidProjectFolder(parentDir.absolutePath()) == false)
		  {
			  QMessageBox::warning(0, tr("Error"), tr("Please select Unity Root Assets Folder."), QMessageBox::Ok);
			  return;
		  }

		  assetsFolderEdit->setText(directoryName);
		  settings->setValue("AssetsPath", directoryName);

	 }
	 // user cancelled, return with no further popups
	 return;
}

void DzUnityDialog::HandleInstallUnityFilesCheckBoxChange(int state)
{
	 settings->setValue("InstallUnityFiles", state == Qt::Checked);
}

void DzUnityDialog::HandleAssetTypeComboChange(int state)
{
	QString assetNameString = assetNameEdit->text();

	// enable/disable Morphs and Subdivision only if Skeletal selected
	if (assetTypeCombo->currentText() != "Skeletal Mesh")
	{
		morphsEnabledCheckBox->setChecked(false);
		subdivisionEnabledCheckBox->setChecked(false);
	}

	// if "Animation", change assetname
	if (assetTypeCombo->currentText() == "Animation")
	{
		// check assetname is in @anim[0000] format
		if (!assetNameString.contains("@") || assetNameString.contains(QRegExp("@anim[0-9]*")))
		{
			// extract true assetName and recompose animString
			assetNameString = assetNameString.left(assetNameString.indexOf("@"));
			// get importfolder using corrected assetNameString
			QString importFolderPath = settings->value("AssetsPath").toString() + QDir::separator() + "Daz3D" + QDir::separator() + assetNameString + QDir::separator();

			// create anim filepath
			uint animCounter = 0;
			QString animString = assetNameString + QString("@anim%1").arg(animCounter, 4, 10, QChar('0'));
			QString filePath = importFolderPath + animString + ".fbx";

			// if anim file exists, then increment anim filename counter
			while (QFileInfo(filePath).exists())
			{
				if (++animCounter > 9999)
				{
					break;
				}
				animString = assetNameString + QString("@anim%1").arg(animCounter, 4, 10, QChar('0'));
				filePath = importFolderPath + animString + ".fbx";
			}
			assetNameEdit->setText(animString);
		}

	}
	else
	{
		// remove @anim if present
		if (assetNameString.contains("@")) {
			assetNameString = assetNameString.left(assetNameString.indexOf("@"));
		}
		assetNameEdit->setText(assetNameString);
	}

}

void DzUnityDialog::HandleTargetPluginInstallerButton()
{
	if (m_wTargetPluginInstaller == nullptr) return;

	// Validate software version and set resource zip files to extract
	QString softwareVersion = m_TargetSoftwareVersionCombo->currentText();
	if (softwareVersion == "" || softwareVersion.toLower().contains("select"))
	{
		// Warning, not a valid plugins folder path
		QMessageBox::information(0, "Daz Bridge",
			tr("Please select a Rendering Pipeline."));
		return;
	}

	// For first run, display help / explanation popup dialog...
	// TODO

	// Get Destination Folder
	QString directoryName = QFileDialog::getExistingDirectory(this,
		tr("Please select a Project Folder to Install the Unity Plugin"),
		"/home",
		QFileDialog::ShowDirsOnly
		| QFileDialog::DontResolveSymlinks);

	if (directoryName == NULL)
	{
		// User hit cancel: return without addition popups
		return;
	}

	// fix path separators
	directoryName.replace("\\", "/");

	// Validate selected Folder is valid for plugin
	bool bIsValidProjectFolder = false;

	if (QRegExp(".*/assets/daz3d$").exactMatch(directoryName.toLower()) == true)
	{
		directoryName.replace("/assets/daz3d", "", Qt::CaseInsensitive);
	}
	else if (QRegExp(".*/assets$").exactMatch(directoryName.toLower()) == true)
	{
		directoryName.replace("/assets","", Qt::CaseInsensitive);
	}
	bIsValidProjectFolder = IsValidProjectFolder(directoryName);

	if (bIsValidProjectFolder == false)
	{
		// Warning, not a valid plugins folder path
		auto userChoice = QMessageBox::warning(0, "Daz To Unity Bridge",
			tr("The selected folder is not a valid Unity Project folder.  Please select a \
valid Unity Project folder.\n\nYou can choose to Abort and select a new folder, \
or Ignore this error and install the plugin anyway."),
QMessageBox::Ignore | QMessageBox::Abort,
QMessageBox::Abort);
		if (userChoice == QMessageBox::StandardButton::Abort)
			return;
	}

	QString sDestinationPath = directoryName + "/Assets/Daz3D/Support";

	// create plugins folder if does not exist
	if (QDir(sDestinationPath).exists() == false)
	{
		QDir().mkpath(sDestinationPath);
	}

	bool bInstallSuccessful = true;
	bool bReplace = true;

	QString srcPathHDRP = ":/DazBridgeUnity/daztounity-hdrp.unitypackage";
	QFile srcFileHDRP(srcPathHDRP);
	QString destPathHDRP = sDestinationPath + "/DazToUnity HDRP.unitypackage";
	if (DZ_BRIDGE_NAMESPACE::DzBridgeAction::copyFile(&srcFileHDRP, &destPathHDRP, bReplace) == false)
		bInstallSuccessful = false;
	srcFileHDRP.close();

	QString srcPathURP = ":/DazBridgeUnity/daztounity-urp.unitypackage";
	QFile srcFileURP(srcPathURP);
	QString destPathURP = sDestinationPath + "/DazToUnity URP.unitypackage";
	if (DZ_BRIDGE_NAMESPACE::DzBridgeAction::copyFile(&srcFileURP, &destPathURP, bReplace) == false)
		bInstallSuccessful = false;
	srcFileURP.close();

	QString srcPathStandard = ":/DazBridgeUnity/daztounity-standard-shader.unitypackage";
	QFile srcFileStandard(srcPathStandard);
	QString destPathStandard = sDestinationPath + "/DazToUnity Standard Shader.unitypackage";
	if (DZ_BRIDGE_NAMESPACE::DzBridgeAction::copyFile(&srcFileStandard, &destPathStandard, bReplace) == false)
		bInstallSuccessful = false;
	srcFileStandard.close();

	QString activePluginPath = "";
	if (softwareVersion.contains("HDRP", Qt::CaseInsensitive))
	{
		activePluginPath = destPathHDRP;
	}
	if (softwareVersion.contains("URP", Qt::CaseInsensitive))
	{
		activePluginPath = destPathURP;
	}
	if (softwareVersion.contains("Built-In", Qt::CaseInsensitive))
	{
		activePluginPath = destPathStandard;
	}

	// verify successful plugin extraction/installation
	if (bInstallSuccessful)
	{
		QMessageBox::information(0, "Daz To Unity Bridge",
			tr("Plugin copied to: ") + activePluginPath +
			tr(".  Please switch to your Unity Project to complete the import process. \
If Unity Import dialog does not appear, then please double-click desired UnityPackage \
file located in the Assets\\Daz3D\\Support\\ folder of your Unity Project."), QMessageBox::Ok);

#ifdef WIN32
		ShellExecute(0, 0, activePluginPath.toLocal8Bit().data(), 0, 0, SW_SHOW);
#endif

	}
	else
	{
		QMessageBox::warning(0, "Daz To Unity Bridge",
			tr("Sorry, an unknown error occured. Unable to install \
Unity Plugin to: ") + sDestinationPath);
		return;
	}

	return;
}

void DzUnityDialog::HandleOpenIntermediateFolderButton(QString sFolderPath)
{
	// Open Selected Unity Project Folder
	QString assetFolder = assetsFolderEdit->text().replace("\\", "/") + "/Daz3D";
	DzBridgeDialog::HandleOpenIntermediateFolderButton(assetFolder);
}

// Return TRUE if Project Folder Path is Valid
bool DzUnityDialog::IsValidProjectFolder(QString sProjectFolderPath)
{
	if (QDir(sProjectFolderPath).exists() == false)
		return false;

	bool found1 = false;
	bool found2 = false;
	bool found3 = false;

	if (QDir(sProjectFolderPath + "/Assets").exists() == true)
	{
		found1 = true;
	}
	if (QDir(sProjectFolderPath + "/ProjectSettings").exists() == true)
	{
		found2 = true;
	}
	if (QDir(sProjectFolderPath + "/Library").exists() == true)
	{
		found3 = true;
	}

	if (found1 && found2 && found3)
	{
		return true;
	}

	return false;
}

void DzUnityDialog::setDisabled(bool bDisabled)
{
	DzBridgeDialog::setDisabled(bDisabled);

	assetsFolderButton->setDisabled(bDisabled);
	assetsFolderEdit->setDisabled(bDisabled);
	installUnityFilesCheckBox->setDisabled(bDisabled);

}

#include "moc_DzUnityDialog.cpp"
