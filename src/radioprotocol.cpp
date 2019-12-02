// Written by Adrian Musceac YO8RZZ , started December 2017.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "radioprotocol.h"

RadioProtocol::RadioProtocol(Logger *logger, QObject *parent) :
    QObject(parent)
{
    _logger = logger;
    _buffer = new QByteArray;
}

void RadioProtocol::processRadioMessage(QByteArray data)
{
    int msg_type;
    unsigned int data_len;
    unsigned int crc;
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> msg_type;
    stream >> data_len;
    if(data_len > 1024*1024)
        return;
    char *tmp = new char[data_len];
    stream >> crc;
    stream.readBytes(tmp, data_len);
    QByteArray message(tmp, data_len);
    delete tmp;

    if(crc != gr::digital::crc32(message.toStdString()))
    {
        _logger->log(Logger::LogLevelCritical, "Radio packet CRC32 failed, dropping packet");
        return;
    }

    switch(msg_type)
    {
    case MsgTypePageMessage:
        processPageMessage(message);
        break;
    case MsgTypeRepeaterInfo:
        processRepeaterInfo(message);
        break;
    default:
        _logger->log(Logger::LogLevelDebug,
                     QString("Radio message type %1 not implemented").arg(msg_type));
        break;
    }

}

QByteArray RadioProtocol::buildRadioMessage(QByteArray data, int msg_type)
{
    unsigned int crc = gr::digital::crc32(data.toStdString());
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << msg_type;
    stream << data.length();
    stream << crc;
    stream << data;
    return message;
}

QByteArray RadioProtocol::buildPageMessage(QString calling_callsign, QString called_callsign,
                                           bool retransmit, QString via_node)
{

    QRadioLink::PageMessage *p = new QRadioLink::PageMessage;
    p->set_calling_user(calling_callsign.toStdString());
    p->set_called_user(called_callsign.toStdString());
    p->set_retransmit(retransmit);
    p->set_via_node(via_node.toStdString());
    int size = p->ByteSize();
    unsigned char data[size];
    p->SerializeToArray(data,size);
    QByteArray msg(reinterpret_cast<const char*>(data));
    delete p;
    return buildRadioMessage(msg, MsgTypePageMessage);
}

QByteArray RadioProtocol::buildRepeaterInfo()
{
    QByteArray data;
    QRadioLink::RepeaterInfo repeater_info;
    for(int i=0;i <_voip_channels.size();i++)
    {
        QRadioLink::RepeaterInfo::Channel *ch = repeater_info.add_channels();
        ch->set_channel_id(_voip_channels.at(i)->id);
        ch->set_parent_id(_voip_channels.at(i)->parent_id);
        ch->set_name(_voip_channels.at(i)->name.toStdString().c_str());
        ch->set_description(_voip_channels.at(i)->description.toStdString().c_str());
    }
    for(int i=0;i <_voip_users.size();i++)
    {
        QRadioLink::RepeaterInfo::User *u = repeater_info.add_users();;
        u->set_user_id(_voip_users.at(i)->id);
        u->set_channel_id(_voip_users.at(i)->channel_id);
        u->set_name(_voip_users.at(i)->callsign.toStdString().c_str());
    }
    char bin[repeater_info.ByteSize()];
    repeater_info.SerializeToArray(bin,repeater_info.ByteSize());
    data.append(bin, repeater_info.ByteSize());
    return buildRadioMessage(data, MsgTypeRepeaterInfo);
}


void RadioProtocol::setStations(QVector<Station *> list)
{

    _voip_users = list;
}

void RadioProtocol::setChannels(QVector<MumbleChannel *> list)
{
    _voip_channels = list;
}

void RadioProtocol::processRepeaterInfo(QByteArray message)
{
    QRadioLink::RepeaterInfo info;
    info.ParseFromArray(message.data(), message.size());
    for(int i=0; i<info.channels_size();i++)
    {
        qDebug() << QString::fromStdString(info.channels(i).name());
    }
    for(int i=0; i<info.users_size();i++)
    {
        qDebug() << QString::fromStdString(info.users(i).name());
    }
}

void RadioProtocol::processPayload(QByteArray data)
{

    int msg_type = data.at(0);
    data = data.mid(1);
    switch(msg_type)
    {
    case 1:
    {
        QRadioLink::Channel ch;
        ch.ParseFromArray(data,data.size());
        qDebug() << QString::fromStdString(ch.name());
        /*
        MumbleChannel *chan = new MumbleChannel(
                    ch.channel_id(),ch.parent_id(),QString::fromStdString(ch.name()),
                    QString::fromStdString(ch.description()));
        emit newChannel(chan);
        */
        break;
    }
    case 2:
    {
        QRadioLink::User u;
        u.ParseFromArray(data,data.size());
        qDebug() << QString::fromStdString(u.name());
        break;
    }
    }
}

void RadioProtocol::processPageMessage(QByteArray message)
{
    QRadioLink::PageMessage page;
    page.ParseFromArray(message.data(), message.size());
    _logger->log(Logger::LogLevelDebug, QString("Paging message from %1 to %2 via %3").arg(
                     QString::fromStdString(page.calling_user())).arg(
                     QString::fromStdString(page.called_user())).arg(
                     QString::fromStdString(page.via_node())));
}


