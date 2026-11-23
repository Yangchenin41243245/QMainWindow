#ifndef MQWINDOW_H
#define MQWINDOW_H

#include <QMainWindow>
#include <QImage>
#include "ui_mqwindow.h"        // uic 由 mqwindow.ui 產生，內含 Ui_MQWindow 類別

// 多重繼承：同時是 QMainWindow，也擁有 Designer 畫好的所有元件
// （actionOpen、menu_F、filetoolBar、imageLabel ... 都可以直接使用）
class MQWindow : public QMainWindow, public Ui_MQWindow
{
    Q_OBJECT

public:
    explicit MQWindow(QWidget *parent = nullptr);
    ~MQWindow() override;

protected:
    // 拖放：滑鼠帶著檔案進入視窗時判斷收不收，放開時載入
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    // 檔案功能表
    void newImage();
    void openImage();
    void closeImage();
    bool saveImage();
    bool saveImageAs();
    // 編輯功能表
    void undo();
    void cutImage();
    void copyImage();
    void pasteImage();

private:
    bool loadImage(const QString &fileName);    // Open 與拖放共用的載入流程
    bool writeImage(const QString &fileName);   // Save 與另存共用的寫檔流程
    void setImage(const QImage &newImage);      // 更新畫面，並把舊圖留給 Undo
    void updateActions();                       // 依目前狀態啟用 / 停用 QAction
    void updateStatus();
    static bool isSupportedImage(const QString &fileName);

    QImage image;            // 目前的影像資料（之後做影像處理用）
    QImage previousImage;    // 上一步的影像，給 Undo 用
    bool canUndo = false;
    QString currentFile;     // 目前圖片的路徑；貼上的圖片沒有路徑
};
#endif // MQWINDOW_H
