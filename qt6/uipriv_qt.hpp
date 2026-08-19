#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <vector>

#include <QApplication>
#include <QAbstractTableModel>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QEvent>
#include <QFileDialog>
#include <QFontDialog>
#include <QFormLayout>
#include <QFrame>
#include <QKeyEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableView>
#include <QTimer>
#include <QToolTip>
#include <QTextCursor>
#include <QTextDocument>
#include <QTransform>
#include <QVBoxLayout>
#include <QWindow>
#include <QWheelEvent>

#include "../ui.h"
#include "../ui_qt.h"
#include "../common/uipriv.h"

extern uiInitOptions uiprivOptions;

constexpr int uiprivQtSpacing = 6;
constexpr int uiprivQtMargin = 12;
constexpr uint32_t uiprivQtControlSignature = 0x51743643;

inline QString qstring(const char *s)
{
	return QString::fromUtf8(s == nullptr ? "" : s);
}

inline char *qtText(const QString &s)
{
	QByteArray b = s.toUtf8();
	char *out = static_cast<char *>(std::malloc(static_cast<size_t>(b.size()) + 1));
	if (out == nullptr)
		return nullptr;
	std::memcpy(out, b.constData(), static_cast<size_t>(b.size()));
	out[b.size()] = '\0';
	return out;
}

inline QWidget *qtWidget(uiControl *c)
{
	return uiQtControl(c)->widget;
}

void uiprivQtInitControl(uiQtControl *c, QWidget *widget, bool owns,
	uint32_t typesig, const char *name, bool toplevel = false);
void uiprivQtDetach(uiControl *c);
void uiprivQtUninitMenus(void);
