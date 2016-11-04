// Headers to include: the ui_fileinsight.h is automatically generated by Qt's uic tool on compile
#include "fileinsight.h"
#include "ui_fileinsight.h"
#include "fileinsightsubdialog.h"
#include "ui_fileinsightsubdialog.h"

// Main window class
FileInsight::FileInsight(QWidget *parent) : QMainWindow(parent), ui(new Ui::FileInsight) {
    // Set up the UI by creating an instance of our UI class and initializing it

    ui->setupUi(this);

#ifdef Q_OS_WIN
    // On Windows, use an absolute path for TrID (in our thirdparty/ folder)
    QString appdir = QCoreApplication::applicationDirPath();
    this->trid_command = appdir + "/thirdparty/trid";

    // Set the path and name of our bundled icon theme
    QIcon::setThemeName("oxygen");
    QIcon::setThemeSearchPaths(QStringList() << appdir + "/icons");
#else
    this->trid_command = "trid";
#endif

    /* Initialize libmagic by fetching ourselves special cookies from magic_open() - this is
     * similar to fetching a specific instance of a class. More information about the libmagic API:
     * https://linux.die.net/man/3/libmagic
     */
    // libmagic flags (e.g. MAGIC_CHECK) go here
    this->magic_cookie = magic_open(MAGIC_CHECK | MAGIC_COMPRESS);

    // Tell libmagic to load the default file type definitions by passing NULL as filename argument
    magic_load(this->magic_cookie, NULL);

    // Repeat the above process for a second instance of libmagic, specifically used to find MIME
    // types. (MAGIC_MIME_TYPE set)
    this->magic_cookie_mime = magic_open(MAGIC_CHECK | MAGIC_MIME_TYPE);
    magic_load(this->magic_cookie_mime, NULL);

    ui->tabWidget->addTab(new FileInsightSubdialog(this), tr("CHANGE THIS"));

}

// Destructor for the FileInsight class:
FileInsight::~FileInsight()
{
    // Delete the temporary ui variable that we assigned in the constructor.
    delete ui;
}

void FileInsight::on_actionQuit_triggered()
{
    // Implement the Quit action in the File menu. This calls quit() on the global "qApp"
    // pointer, which refers to the current running QApplication instance
    qApp->quit();
}

void FileInsight::chooseFile()
{
    // This implements file selection via Qt's file dialog built-ins
    QString filename = QFileDialog::getOpenFileName(this, tr("Select File"), QString(),
            tr("All Files (*)"));
    std::cout << "Selected file: " << filename.toStdString() << std::endl;

    if (!filename.isEmpty()) {
        // Open the file afterwards.
        this->openFile(filename);
    }
}

QString FileInsight::getMagicInfo(QString filename)
{
    // Call libmagic on the filename - it will return a string describing the file.
    QString magic_output = magic_file(this->magic_cookie, this->QStringToConstChar(filename));
    return magic_output;
}


QString FileInsight::getTridInfo(QString filename)
{
    // This method gets extended file info using the TrID command line program,
    // by running it in a subprocess.

    QString data;

    // FIXME: this breaks if there's a - in the filename
    this->trid_subprocess.start(this->trid_command, QStringList() << "\"" << filename << "\"");

    QByteArray result;
    // Grab all of the subprocess' text output in a loop.
    while (this->trid_subprocess.waitForReadyRead())
    {
        result += this->trid_subprocess.readAll();
    }
    data = result.data();
    // Trim to remove leading & trailing whitespace
    data = data.trimmed();

    if (data.isEmpty())
    {
        // The TrID process failed to start or otherwise returned no data.
        // Raise an error.
        QMessageBox::critical(this, tr("Failed to start TrID"),
                             tr("An error occurred while retrieving data from the TrID backend. "
                                "Check that TrID is installed and in your PATH!"));
    }

    return data;
}

QString FileInsight::getMimeType(QString filename)
{
    /* A second cookie (libmagic initialized with different options) allows us to fetch the MIME
     * type of the file instead of the description.
     */

    QString mimetype;
    int backend = this->getBackend();
    if (backend == BACKEND_QT || backend == BACKEND_QT_FILEONLY) {
        // Qt5 / QMimeDatabase backend
        QMimeDatabase mimedb;
        std::cout << "getMimeType(): Using Qt5/QMimeDatabase backend" << std::endl;

        QMimeType mimeobj;
        if (backend == BACKEND_QT_FILEONLY)
        {
            mimeobj = mimedb.mimeTypeForFile(filename, QMimeDatabase::MatchContent);
        } else {
            mimeobj = mimedb.mimeTypeForFile(filename);
        }
        mimetype = mimeobj.name();
    } else {
        // libmagic MIME type backend
        std::cout << "getMimeType(): Using libmagic backend" << std::endl;
        // Fetch the MIME type for the given file: this allows us to fetch an icon for it.
        mimetype = magic_file(this->magic_cookie_mime, this->QStringToConstChar(filename));
    }
    return mimetype;
}

const char * FileInsight::QStringToConstChar(QString text) {
    QByteArray bytes = text.toUtf8();
    return bytes.constData();
}

int FileInsight::getBackend() {
    // Returns the backend currently in use.
    if (ui->backend_magic->isChecked()) {
        return BACKEND_MAGIC;
    } else if (ui->backend_trid->isChecked()) {
        return BACKEND_TRID;
    } else if (ui->backend_qt->isChecked()) {
        return BACKEND_QT;
    } else if (ui->backend_qt_fileonly->isChecked()) {
        return BACKEND_QT_FILEONLY;
    }
}

FileInsightSubdialog* FileInsight::getCurrentTab() {
    return (FileInsightSubdialog *) ui->tabWidget->currentWidget();
}

void FileInsight::openFile(QString filename)
{
    //FileInsightSubdialog *currentTab = (FileInsightSubdialog *) ui->tabWidget->currentWidget();
    FileInsightSubdialog *currentTab = this->getCurrentTab();
    /* Convert QString into const char *, so that it can be plugged into the libmagic C library
     * Note: the QByteArray created by toUtf8() must be kept as a variable and not destroyed, or
     * the pointer returned by constData() may become invalid.
     */
    currentTab->ui->filenameOutput->setPlainText(filename);
    currentTab->filename = filename;

    QString ext_info;
    int backend = this->getBackend();
    // Fill in extended info: use TrID or libmagic backends (whichever is selected)
    if (backend == BACKEND_TRID)
    {
        ext_info = this->getTridInfo(filename);
    } else if (backend == BACKEND_MAGIC) {
        ext_info = this->getMagicInfo(filename);
    }
    currentTab->ui->output->setPlainText(ext_info);

    // Get the MIME type and use it to fetch the icon
    QString mimetype = this->getMimeType(filename);
    currentTab->ui->mimeOutput->setPlainText(mimetype);

    QIcon icon = this->getIcon(mimetype);
    currentTab->ui->iconDisplay->setPixmap(icon.pixmap(128,128));
}

QIcon FileInsight::getIcon(QString mimetype) {
    QIcon icon;
    // On *nix systems, the icon name is derived from the mime type.
    QString iconname = mimetype;
    //std::cout << "errors?: " << magic_error(this->magic_cookie_mime);

    /* Generic MIME types are created by taking first part of the type (e.g. "video" from "video/ogg")
     * and adding "-x-generic" to it. So, the generic type for video/ogg would be video-x-generic.
     */
    QString generic_type = mimetype.split("/")[0] + "-x-generic";

    // Replace any "/" with "-" in the MIME type before icon lookup.
    int slashlocation = iconname.indexOf("/");
    if (slashlocation != -1) {
        iconname.replace(slashlocation, 1, "-");
    }

    std::cout << "Looking up icon for MIME type " << mimetype.toStdString() <<
                 " (generic name: " << generic_type.toStdString() << ")" << std::endl;

    if (QIcon::hasThemeIcon(iconname)) {
        /* If the icon theme we're using has an icon for the MIME type we're looking for,
         * use that.
         */
        icon = QIcon::fromTheme(iconname);
    } else {
        /* Otherwise, fall back to the following in order:
         * 1) The icon for the generic type (e.g. a "video" icon for .mp4 files)
         * 2) The unknown file type icon in the icon theme used
         * 3) Qt's (small) generic file icon.
         */
        if (QIcon::hasThemeIcon(generic_type)) {
            icon = QIcon::fromTheme(generic_type);
        } else {
            icon = QIcon::fromTheme("unknown", this->iconprovider.icon(QFileIconProvider::File));
        }
    }
    return icon;
}

void FileInsight::on_selectFileButton_clicked()
{
    this->chooseFile();
}

void FileInsight::on_actionSelect_triggered()
{
    this->chooseFile();
}

void FileInsight::on_reloadButton_clicked()
{
    FileInsightSubdialog *currentTab = this->getCurrentTab();
    if (!currentTab->filename.isEmpty()) {
        this->openFile(currentTab->filename);
     } else {
        QMessageBox::critical(this, tr("No file selected"), tr("You must select a file before reloading!"));
    }
}
