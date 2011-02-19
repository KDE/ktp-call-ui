/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef QTF_H
#define QTF_H

/* This module is a wrapper for telepathy-farstream that
 * uses the QtGLib/QtGStreamer bindings as a base. It could
 * be built standalone and used in other projects in the future.
 */

#include <QGlib/Object>
#include <QGst/Message>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4Yell/CallChannel>

typedef struct _TfChannel TfChannel;
typedef struct _TfContent TfContent;

//for the moment, this is a static lib
#define QTF_EXPORT

#define QTF_WRAPPER(Class) \
    QGLIB_WRAPPER_DECLARATION_MACRO(Class, Class, Tf, Class)

#define QTF_REGISTER_TYPE(Class) \
    QGLIB_REGISTER_TYPE_WITH_EXPORT_MACRO(Class, QTF_EXPORT)

namespace QTf {

class Channel;
typedef QGlib::RefPointer<Channel> ChannelPtr;
class Content;
typedef QGlib::RefPointer<Content> ContentPtr;


/*! Wrapper for TfChannel */
class QTF_EXPORT Channel : public QGlib::Object
{
    QTF_WRAPPER(Channel)
public:
    bool processBusMessage(const QGst::MessagePtr & message);
};


/*! Wrapper for TfContent */
class QTF_EXPORT Content : public QGlib::Object
{
    QTF_WRAPPER(Content)
};


/*! Initializes telepathy-farstream and registers the
 * QTf wrapper types with the QGlib type system */
QTF_EXPORT void init();


/*! Constructs a new QTf::Channel from a Tpy::CallChannel */
class QTF_EXPORT PendingChannel : public Tp::PendingOperation
{
    Q_OBJECT
public:
    PendingChannel(const Tpy::CallChannelPtr & callChannel);
    virtual ~PendingChannel();

    QTf::ChannelPtr channel() const;

private Q_SLOTS:
    void onPendingTfChannelFinished(Tp::PendingOperation *op);

private:
    QTf::ChannelPtr m_channel;
};


} //namespace QTf

QTF_REGISTER_TYPE(QTf::Channel)
QTF_REGISTER_TYPE(QTf::Content)

#endif // QTF_H
