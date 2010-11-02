
#include <QGLWidget>
#include <QDebug>
#include <QDomNode>
#include <QEvent>
#include <QDragEnterEvent>
#include <QUrl>
#include <QPainter>
#include <QFile>


#include "mixxx.h"
#include "trackinfoobject.h"
#include "wglwaveformviewer.h"
#include "waveform/waveformrenderer.h"

WGLWaveformViewer::WGLWaveformViewer(const char *group, WaveformRenderer *pWaveformRenderer, QWidget * pParent, const QGLWidget * pShareWidget, Qt::WFlags f) : QGLWidget(QGLFormat(QGL::SampleBuffers), pParent, pShareWidget)
{

    m_pWaveformRenderer = pWaveformRenderer;
    Q_ASSERT(m_pWaveformRenderer);

    m_pGroup = group;

    setAcceptDrops(true);

    installEventFilter(this);

    m_painting = false;
}

bool WGLWaveformViewer::directRendering()
{
    return format().directRendering();
}


WGLWaveformViewer::~WGLWaveformViewer() {
}

void WGLWaveformViewer::setup(QDomNode node) {
    int w = width(), h = height();
    m_pWaveformRenderer->setup(node);
    m_pWaveformRenderer->resize(w, h);
}

void WGLWaveformViewer::paintEvent(QPaintEvent *event) {
    QPainter painter;
    painter.begin(this);

    painter.setRenderHint(QPainter::Antialiasing);
    //painter.setRenderHint(QPainter::TextAntialiasing);

    // HighQualityAntialiasing makes some CPUs go crazy
    //painter.setRenderHint(QPainter::HighQualityAntialiasing);

    m_pWaveformRenderer->draw(&painter, event);

    painter.end();
    m_painting = false;
    // QPainter goes out of scope and is destructed
}

void WGLWaveformViewer::refresh() {
    //m_paintMutex.lock();
    if(!m_painting) {
        m_painting = true;

        // The docs say update is better than repaint.
        update();
        //updateGL();
    }
    //m_paintMutex.unlock();
}

/** SLOTS **/

void WGLWaveformViewer::setValue(double) {
    // unused, stops a bad connect from happening
}



bool WGLWaveformViewer::eventFilter(QObject *o, QEvent *e) {
    if(e->type() == QEvent::MouseButtonPress) {
        QMouseEvent *m = (QMouseEvent*)e;
        m_iMouseStart= -1;
        if(m->button() == Qt::LeftButton) {
            // The left button went down, so store the start position
            m_iMouseStart = m->x();
            emit(valueChangedLeftDown(64));
        }
    } else if(e->type() == QEvent::MouseMove) {
        // Only send signals for mouse moving if the left button is pressed
        if(m_iMouseStart != -1) {
            QMouseEvent *m = (QMouseEvent*)e;

            // start at the middle of 0-127, and emit values based on
            // how far the mouse has travelled horizontally
            double v = 64 + (double)(m->x()-m_iMouseStart)/10;
            // clamp to 0-127
            if(v<0)
                v = 0;
            else if(v > 127)
                v = 127;
            emit(valueChangedLeftDown(v));

        }
    } else if(e->type() == QEvent::MouseButtonRelease) {
        emit(valueChangedLeftDown(64));
    } else {
        return QObject::eventFilter(o,e);
    }
    return true;
}

/** DRAG AND DROP **/


void WGLWaveformViewer::dragEnterEvent(QDragEnterEvent * event)
{
    // Accept the enter event if the thing is a filepath.
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void WGLWaveformViewer::dropEvent(QDropEvent * event)
{
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls(event->mimeData()->urls());
        QUrl url = urls.first();
        QString name = url.toLocalFile();
		//total OWEN hack: because we strip out the library prefix
		//in the view, we have to add it back here again to properly receive
		//drops
        if (!QFile(name).exists())
        {
        	if(QFile(m_sPrefix+"/"+name).exists())
        		name = m_sPrefix+"/"+name;
        }
        //If the file is on a network share, try just converting the URL to a string...
        if (name == "")
            name = url.toString();

        event->accept();
        emit(trackDropped(name, m_pGroup));
    } else {
        event->ignore();
    }
}

void WGLWaveformViewer::setLibraryPrefix(QString sPrefix)
{
	m_sPrefix = "";
	m_sPrefix = sPrefix;
	if (sPrefix[sPrefix.length()-1] == '/' || sPrefix[sPrefix.length()-1] == '\\')
		m_sPrefix.chop(1);
}
