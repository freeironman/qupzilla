/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2016  David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "webinspector.h"
#include "mainapplication.h"
#include "networkmanager.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>

QList<QWebEngineView*> WebInspector::s_views;

WebInspector::WebInspector(QWidget *parent)
    : QWebEngineView(parent)
    , m_view(Q_NULLPTR)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QSL("web-inspector"));
    setMinimumHeight(80);

    registerView(this);

    connect(page(), &QWebEnginePage::windowCloseRequested, this, &WebInspector::deleteLater);
    connect(page(), &QWebEnginePage::loadFinished, this, &WebInspector::loadFinished);
}

WebInspector::~WebInspector()
{
    unregisterView(this);
}

void WebInspector::setView(QWebEngineView *view)
{
    m_view = view;
    Q_ASSERT(isEnabled());

    int port = qEnvironmentVariableIntValue("QTWEBENGINE_REMOTE_DEBUGGING");
    QUrl inspectorUrl = QUrl(QSL("http://localhost:%1").arg(port));
    int index = s_views.indexOf(m_view);

    QNetworkReply *reply = mApp->networkManager()->get(QNetworkRequest(inspectorUrl.resolved(QUrl("json/list"))));
    connect(reply, &QNetworkReply::finished, this, [=]() {
        QJsonArray clients = QJsonDocument::fromJson(reply->readAll()).array();
        QUrl pageUrl;
        if (clients.size() > index) {
            QJsonObject object = clients.at(index).toObject();
            pageUrl = inspectorUrl.resolved(QUrl(object.value(QSL("devtoolsFrontendUrl")).toString()));
        }
        load(pageUrl);
        pushView(this);
        show();
    });
}

void WebInspector::inspectElement()
{
    m_inspectElement = true;
}

bool WebInspector::isEnabled()
{
    return qEnvironmentVariableIsSet("QTWEBENGINE_REMOTE_DEBUGGING");
}

void WebInspector::pushView(QWebEngineView *view)
{
    s_views.removeOne(view);
    s_views.prepend(view);
}

void WebInspector::registerView(QWebEngineView *view)
{
    s_views.prepend(view);
}

void WebInspector::unregisterView(QWebEngineView *view)
{
    s_views.removeOne(view);
}

void WebInspector::loadFinished()
{
    // Show close button only when docked
    if (!isWindow()) {
        page()->runJavaScript(QL1S("var toolbar = document.getElementsByClassName('inspector-view-toolbar')[1];"
                                   "var button = document.createElement('button');"
                                   "button.style.width = '22px';"
                                   "button.style.height = '22px';"
                                   "button.style.border = 'none';"
                                   "button.style.cursor = 'pointer';"
                                   "button.style.background = 'url(qrc:html/close.png) no-repeat';"
                                   "button.style.backgroundPosition = 'center center';"
                                   "button.addEventListener('click', function() {"
                                   "    window.close();"
                                   "});"
                                   "toolbar.appendChild(button);"));
    }

    // Inspect element
    if (m_inspectElement) {
        m_view->triggerPageAction(QWebEnginePage::InspectElement);
        m_inspectElement = false;
    }
}

void WebInspector::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event)
    // Stop propagation
}

void WebInspector::keyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event)
    // Stop propagation
}
