#include "mqwindow.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QMessageBox>
#include <QMimeData>
#include <QPixmap>
#include <QUrl>

MQWindow::MQWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);              // 建立 Designer 畫的選單、工具列、動作物件、centralWidget

    // 每個 QAction 只連一次；不論從功能表、工具列或快捷鍵觸發，都進入同一個槽
    connect(actionNew,   &QAction::triggered, this, &MQWindow::newImage);
    connect(actionOpen,  &QAction::triggered, this, &MQWindow::openImage);
    connect(actionClose, &QAction::triggered, this, &MQWindow::closeImage);
    connect(actionSave,  &QAction::triggered, this, &MQWindow::saveImage);
    connect(actionASave, &QAction::triggered, this, &MQWindow::saveImageAs);
    connect(actionQuit,  &QAction::triggered, this, &QWidget::close);

    connect(actionUndo,  &QAction::triggered, this, &MQWindow::undo);
    connect(actionCuT,   &QAction::triggered, this, &MQWindow::cutImage);
    connect(actionCopy,  &QAction::triggered, this, &MQWindow::copyImage);
    connect(actionPaste, &QAction::triggered, this, &MQWindow::pasteImage);

    // 全選對整張圖片沒有意義，第一版先停用；之後做「選取區域」時再實作
    actionAll->setEnabled(false);

    setAcceptDrops(true);       // 整個主視窗都可以接收拖放
    updateActions();
    statusbar->showMessage("Ready");
}

MQWindow::~MQWindow() = default;

// ---------------- 檔案功能表 ----------------

void MQWindow::newImage()
{
    closeImage();               // 影像程式的「新增」= 清出一個空白工作區
}

void MQWindow::openImage()
{
    // 依照 Qt 實際支援的格式組出篩選字串，例如 *.png *.jpg *.bmp ...
    QStringList patterns;
    for (const QByteArray &fmt : QImageReader::supportedImageFormats())
        patterns << "*." + QString::fromLatin1(fmt);

    const QString fileName = QFileDialog::getOpenFileName(
        this, "開啟圖片", QDir::homePath(),
        QString("圖片檔 (%1);;所有檔案 (*)").arg(patterns.join(' ')));

    if (!fileName.isEmpty())    // 按取消會回傳空字串
        loadImage(fileName);
}

void MQWindow::closeImage()
{
    if (image.isNull())
        return;
    setImage(QImage());         // 清掉圖片，但仍可 Undo 回來
    currentFile.clear();
    updateStatus();
}

bool MQWindow::saveImage()
{
    if (currentFile.isEmpty())  // 貼上的圖片還沒有檔名，改走另存
        return saveImageAs();
    return writeImage(currentFile);
}

bool MQWindow::saveImageAs()
{
    const QString start = currentFile.isEmpty()
                              ? QDir::home().filePath("untitled.png")
                              : currentFile;
    const QString fileName = QFileDialog::getSaveFileName(
        this, "另存圖片", start,
        "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)");

    if (fileName.isEmpty())
        return false;
    if (!writeImage(fileName))
        return false;

    currentFile = fileName;
    updateStatus();
    return true;
}

bool MQWindow::writeImage(const QString &fileName)
{
    QImageWriter writer(fileName);
    if (!writer.write(image)) {
        QMessageBox::warning(this, "無法儲存",
                             QString("無法寫入 %1：\n%2")
                                 .arg(QDir::toNativeSeparators(fileName), writer.errorString()));
        return false;
    }
    statusbar->showMessage(QString("已儲存 %1").arg(QFileInfo(fileName).fileName()), 3000);
    return true;
}

// ---------------- 編輯功能表 ----------------

void MQWindow::undo()
{
    if (!canUndo)
        return;
    std::swap(image, previousImage);    // 再按一次 Undo 可以換回來（Redo 的效果）
    imageLabel->setPixmap(QPixmap::fromImage(image));
    if (image.isNull())
        imageLabel->setText("把圖片拖進來，或按 Ctrl+O 開啟");
    updateActions();
    updateStatus();
}

void MQWindow::copyImage()
{
    if (!image.isNull())
        QApplication::clipboard()->setImage(image);
}

void MQWindow::cutImage()
{
    copyImage();
    closeImage();
}

void MQWindow::pasteImage()
{
    const QImage pasted = QApplication::clipboard()->image();
    if (pasted.isNull()) {
        statusbar->showMessage("剪貼簿裡沒有圖片", 3000);
        return;
    }
    setImage(pasted);
    currentFile.clear();        // 貼上的圖片沒有檔名，Save 會改成另存
    updateStatus();
}

// ---------------- 拖放 ----------------

bool MQWindow::isSupportedImage(const QString &fileName)
{
    const QByteArray suffix = QFileInfo(fileName).suffix().toLower().toLatin1();
    return QImageReader::supportedImageFormats().contains(suffix);
}

void MQWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // 只有「本機檔案」而且是支援的圖片格式才接受，否則游標會顯示禁止符號
    const QMimeData *mime = event->mimeData();
    if (mime->hasUrls()) {
        const QList<QUrl> urls = mime->urls();
        if (!urls.isEmpty() && urls.first().isLocalFile()
            && isSupportedImage(urls.first().toLocalFile())) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void MQWindow::dropEvent(QDropEvent *event)
{
    // 一次拖很多張時，只開第一張
    const QString fileName = event->mimeData()->urls().first().toLocalFile();
    if (loadImage(fileName))
        event->acceptProposedAction();
}

// ---------------- 共用 ----------------

bool MQWindow::loadImage(const QString &fileName)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);      // 手機照片依 EXIF 自動轉正
    const QImage newImage = reader.read();

    if (newImage.isNull()) {
        QMessageBox::warning(this, "無法開啟",
                             QString("無法讀取 %1：\n%2")
                                 .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }

    setImage(newImage);
    currentFile = fileName;
    updateStatus();
    return true;
}

void MQWindow::setImage(const QImage &newImage)
{
    previousImage = image;              // 舊圖留給 Undo
    canUndo = true;
    image = newImage;

    if (image.isNull()) {
        imageLabel->clear();
        imageLabel->setText("把圖片拖進來，或按 Ctrl+O 開啟");
    } else {
        imageLabel->setPixmap(QPixmap::fromImage(image));   // QImage 存資料，QPixmap 負責畫面
    }
    updateActions();
}

void MQWindow::updateActions()
{
    const bool hasImage = !image.isNull();
    actionClose->setEnabled(hasImage);
    actionSave->setEnabled(hasImage);
    actionASave->setEnabled(hasImage);
    actionCuT->setEnabled(hasImage);
    actionCopy->setEnabled(hasImage);
    actionUndo->setEnabled(canUndo);
    // 同一個 QAction 停用後，功能表和工具列上的按鈕會一起變灰
}

void MQWindow::updateStatus()
{
    const QString name = currentFile.isEmpty() ? QString("未命名")
                                               : QFileInfo(currentFile).fileName();
    if (image.isNull()) {
        setWindowTitle("Image Studio");
        statusbar->showMessage("Ready");
    } else {
        setWindowTitle(QString("Image Studio - %1").arg(name));
        statusbar->showMessage(QString("%1 | %2 × %3 px")
                                   .arg(name).arg(image.width()).arg(image.height()));
    }
}
