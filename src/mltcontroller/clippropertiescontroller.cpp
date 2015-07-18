/*
Copyright (C) 2015  Jean-Baptiste Mardelle <jb@kdenlive.org>
This file is part of Kdenlive. See www.kdenlive.org.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy 
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "clippropertiescontroller.h"
#include "bincontroller.h"
#include "timecodedisplay.h"
#include "clipcontroller.h"
#include "kdenlivesettings.h"
#include "effectstack/widgets/choosecolorwidget.h"
#include "dialogs/profilesdialog.h"

#include <KLocalizedString>

#include <QUrl>
#include <QDebug>
#include <QPixmap>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QFontDatabase>

ClipPropertiesController::ClipPropertiesController(Timecode tc, ClipController *controller, QWidget *parent) : QTabWidget(parent)
    , m_controller(controller)
    , m_tc(tc)
    , m_id(controller->clipId())
    , m_type(controller->clipType())
    , m_properties(controller->properties())
{
    setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    m_forcePage = new QWidget(this);
    m_propertiesPage = new QWidget(this);
    m_markersPage = new QWidget(this);

    // Clip properties
    QVBoxLayout *propsBox = new QVBoxLayout;
    QTreeWidget *propsTree = new QTreeWidget;
    propsTree->setRootIsDecorated(false);
    propsTree->setColumnCount(2);
    propsTree->setAlternatingRowColors(true);
    propsTree->setHeaderHidden(true);
    propsBox->addWidget(propsTree);
    fillProperties(propsTree);
    m_propertiesPage->setLayout(propsBox);

    // Clip markers
    QVBoxLayout *mBox = new QVBoxLayout;
    m_markerTree = new QTreeWidget;
    m_markerTree->setRootIsDecorated(false);
    m_markerTree->setColumnCount(2);
    m_markerTree->setAlternatingRowColors(true);
    m_markerTree->setHeaderHidden(true);
    mBox->addWidget(m_markerTree);
    slotFillMarkers();
    m_markersPage->setLayout(mBox);
    connect(m_markerTree, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotEditMarker()));
    
    // Force properties
    QVBoxLayout *vbox = new QVBoxLayout;
    if (m_type == Color || m_type == Image || m_type == AV || m_type == Video) {
        // Edit duration widget
        m_originalProperties.insert("out", m_properties.get("out"));
        m_originalProperties.insert("length", m_properties.get("length"));
        QHBoxLayout *hlay = new QHBoxLayout;
        QCheckBox *box = new QCheckBox(i18n("Duration"), this);
        box->setObjectName("force_duration");
        hlay->addWidget(box);
        TimecodeDisplay *timePos = new TimecodeDisplay(tc, this);
        timePos->setObjectName("force_duration_value");
        timePos->setValue(m_properties.get_int("out") + 1);
        int original_length = m_properties.get_int("kdenlive:original_length");
        if (original_length > 0) {
            box->setChecked(true);
        }
        else timePos->setEnabled(false);
        hlay->addWidget(timePos);
        vbox->addLayout(hlay);
        connect(box, SIGNAL(toggled(bool)), timePos, SLOT(setEnabled(bool)));
        connect(box, SIGNAL(stateChanged(int)), this, SLOT(slotEnableForce(int)));
        connect(timePos, SIGNAL(timeCodeEditingFinished(int)), this, SLOT(slotDurationChanged(int)));
        connect(this, SIGNAL(updateTimeCodeFormat()), timePos, SLOT(slotUpdateTimeCodeFormat()));
        connect(this, SIGNAL(modified(int)), timePos, SLOT(setValue(int)));
    }
    if (m_type == Color) {
        // Edit color widget
        m_originalProperties.insert("resource", m_properties.get("resource"));
        mlt_color color = m_properties.get_color("resource");
        ChooseColorWidget *choosecolor = new ChooseColorWidget(i18n("Color"), QColor::fromRgb(color.r, color.g, color.b).name(), false, this);
        vbox->addWidget(choosecolor);
        //connect(choosecolor, SIGNAL(displayMessage(QString,int)), this, SIGNAL(displayMessage(QString,int)));
        connect(choosecolor, SIGNAL(modified(QColor)), this, SLOT(slotColorModified(QColor)));
        connect(this, SIGNAL(modified(QColor)), choosecolor, SLOT(slotColorModified(QColor)));
    }
    if (m_type == Image) {
        int transparency = m_properties.get_int("kdenlive:transparency");
        m_originalProperties.insert("kdenlive:transparency", QString::number(transparency));
        QHBoxLayout *hlay = new QHBoxLayout;
        QCheckBox *box = new QCheckBox(i18n("Transparent"), this);
        box->setObjectName("kdenlive:transparency");
        box->setChecked(transparency == 1);
        connect(box, SIGNAL(stateChanged(int)), this, SLOT(slotEnableForce(int)));
        hlay->addWidget(box);
        vbox->addLayout(hlay);
    }
    if (m_type == AV || m_type == Video) {
        QLocale locale;
        QString force_fps = m_properties.get("force_fps");
        m_originalProperties.insert("force_fps", force_fps.isEmpty() ? "-" : force_fps);
        QHBoxLayout *hlay = new QHBoxLayout;
        QCheckBox *box = new QCheckBox(i18n("Frame rate"), this);
        box->setObjectName("force_fps");
        connect(box, SIGNAL(stateChanged(int)), this, SLOT(slotEnableForce(int)));
        QDoubleSpinBox *spin = new QDoubleSpinBox(this);
        connect(spin, SIGNAL(valueChanged(double)), this, SLOT(slotValueChanged(double)));
        spin->setObjectName("force_fps_value");
        if (force_fps.isEmpty()) {
            spin->setValue(controller->originalFps());
        }
        else {
            spin->setValue(locale.toDouble(force_fps));
        }
        connect(box, SIGNAL(toggled(bool)), spin, SLOT(setEnabled(bool)));
        box->setChecked(!force_fps.isEmpty());
        spin->setEnabled(!force_fps.isEmpty());
        hlay->addWidget(box);
        hlay->addWidget(spin);
        vbox->addLayout(hlay);

        hlay = new QHBoxLayout;
        box = new QCheckBox(i18n("Colorspace"), this);
        box->setObjectName("force_colorspace");
        connect(box, SIGNAL(stateChanged(int)), this, SLOT(slotEnableForce(int)));
        QComboBox *combo = new QComboBox(this);
        combo->setObjectName("force_colorspace_value");
        combo->addItem(ProfilesDialog::getColorspaceDescription(601), 601);
        combo->addItem(ProfilesDialog::getColorspaceDescription(709), 709);
        combo->addItem(ProfilesDialog::getColorspaceDescription(240), 240);
        int force_colorspace = m_properties.get_int("force_colorspace");
        m_originalProperties.insert("force_colorspace", force_colorspace == 0 ? "-" : QString::number(force_colorspace));
        int colorspace = controller->videoCodecProperty("colorspace").toInt();
        if (force_colorspace > 0) {
            box->setChecked(true);
            combo->setEnabled(true);
            combo->setCurrentIndex(combo->findData(force_colorspace));
        } else if (colorspace > 0) {
            combo->setEnabled(false);
            combo->setCurrentIndex(combo->findData(colorspace));
        }
        else combo->setEnabled(false);
        connect(box, SIGNAL(toggled(bool)), combo, SLOT(setEnabled(bool)));
        hlay->addWidget(box);
        hlay->addWidget(combo);
        
        vbox->addLayout(hlay);
    }
    m_forcePage->setLayout(vbox);
    vbox->addStretch(10);
    addTab(m_propertiesPage, i18n("Properties"));
    addTab(m_markersPage, i18n("Markers"));
    addTab(m_forcePage, i18n("Force"));
    setCurrentIndex(KdenliveSettings::properties_panel_page());
    if (m_type == Color) setTabEnabled(0, false);
}

ClipPropertiesController::~ClipPropertiesController()
{
    KdenliveSettings::setProperties_panel_page(currentIndex());
}

void ClipPropertiesController::slotRefreshTimeCode()
{
    emit updateTimeCodeFormat();
}

void ClipPropertiesController::slotReloadProperties()
{
    if (m_type == Color) {
        m_originalProperties.insert("resource", m_properties.get("resource"));
        m_originalProperties.insert("out", m_properties.get("out"));
        m_originalProperties.insert("length", m_properties.get("length"));
        emit modified(m_properties.get_int("length"));
        mlt_color color = m_properties.get_color("resource");
        emit modified(QColor::fromRgb(color.r, color.g, color.b));
    }
}

void ClipPropertiesController::slotColorModified(QColor newcolor)
{
    QMap <QString, QString> properties;
    properties.insert("resource", newcolor.name(QColor::HexArgb));
    emit updateClipProperties(m_id, m_originalProperties, properties);
    m_originalProperties = properties;
}

void ClipPropertiesController::slotDurationChanged(int duration)
{
    QMap <QString, QString> properties;
    int original_length = m_properties.get_int("kdenlive:original_length");
    if (original_length == 0) {
        m_properties.set("kdenlive:original_length", m_properties.get_int("length"));
    }
    properties.insert("length", QString::number(duration));
    properties.insert("out", QString::number(duration - 1));
    emit updateClipProperties(m_id, m_originalProperties, properties);
    m_originalProperties = properties;
}

void ClipPropertiesController::slotEnableForce(int state)
{
    QCheckBox *box = qobject_cast<QCheckBox *>(sender());
    if (!box) {
        return;
    }
    QString param = box->objectName();
    QMap <QString, QString> properties;
    QLocale locale;
    if (state == Qt::Unchecked) {
        // The force property was disable, remove it / reset default if necessary
        if (param == "force_duration") {
            // special case, reset original duration
            TimecodeDisplay *timePos = findChild<TimecodeDisplay *>(param + "_value");
            timePos->setValue(m_properties.get_int("kdenlive:original_length"));
            slotDurationChanged(m_properties.get_int("kdenlive:original_length"));
            m_properties.set("kdenlive:original_length", (char *) NULL);
            return;
        }
        else if (param == "kdenlive:transparency") {
            properties.insert(param, QString());
        }
        else {
            properties.insert(param, "-");
        }
    } else {
        // A force property was set
        if (param == "force_duration") {
            int original_length = m_properties.get_int("kdenlive:original_length");
            if (original_length == 0) {
                m_properties.set("kdenlive:original_length", m_properties.get_int("length"));
            }
        }
        else if (param == "force_fps") {
            QDoubleSpinBox *spin = findChild<QDoubleSpinBox *>(param + "_value");
            if (!spin) return;
            properties.insert(param, locale.toString(spin->value()));
        }
        else if (param == "force_colorspace") {
            QComboBox *combo = findChild<QComboBox *>(param + "_value");
            if (!combo) return;
            properties.insert(param, QString::number(combo->currentData().toInt()));
        }
        else if (param == "kdenlive:transparency") {
            properties.insert(param, "1");
        }
    }
    if (properties.isEmpty()) return;
    emit updateClipProperties(m_id, m_originalProperties, properties);
    m_originalProperties = properties;
}

void ClipPropertiesController::slotValueChanged(double value)
{
    QDoubleSpinBox *box = qobject_cast<QDoubleSpinBox *>(sender());
    if (!box) {
        return;
    }
    QString param = box->objectName().section("_", 0, -2);
    QMap <QString, QString> properties;
    QLocale locale;
    properties.insert(param, locale.toString(value));
    emit updateClipProperties(m_id, m_originalProperties, properties);
    m_originalProperties = properties;
}


void ClipPropertiesController::fillProperties(QTreeWidget *tree)
{
    QList <QStringList> propertyMap;
    char property[200];
    // Get the video_index
    if (m_type == AV || m_type == Video) {
        int vindex = m_controller->int_property("video_index");
        int video_max = 0;
        int default_audio = m_controller->int_property("audio_index");
        int audio_max = 0;

        // Find maximum stream index values
        for (int ix = 0; ix < m_controller->int_property("meta.media.nb_streams"); ++ix) {
            snprintf(property, sizeof(property), "meta.media.%d.stream.type", ix);
            QString type = m_controller->property(property);
            if (type == "video")
                video_max = ix;
            else if (type == "audio")
                audio_max = ix;
        }
        /*propertyMap["default_video"] = QString::number(vindex);
        propertyMap["video_max"] = QString::number(video_max);
        propertyMap["default_audio"] = QString::number(default_audio);
        propertyMap["audio_max"] = QString::number(audio_max);*/

        if (vindex > -1) {
            // We have a video stream
            snprintf(property, sizeof(property), "meta.media.%d.codec.long_name", vindex);
            QString codec = m_controller->property(property);
            if (!codec.isEmpty()) {
                propertyMap.append(QStringList() << i18n("Video codec") << codec);
            }
            int width = m_controller->int_property("meta.media.width");
            int height = m_controller->int_property("meta.media.height");
            propertyMap.append(QStringList() << i18n("Frame size") << QString::number(width) + "x" + QString::number(height));
            
            snprintf(property, sizeof(property), "meta.media.%d.stream.frame_rate", vindex);
            double fps = m_controller->double_property(property);
            if (fps > 0) {
                    propertyMap.append(QStringList() << i18n("Frame rate") << QString::number(fps, 'g', 3));
            } else {
                int rate_den = m_controller->int_property("meta.media.frame_rate_den");
                if (rate_den > 0) {
                    double fps = (double) m_controller->int_property("meta.media.frame_rate_num") / rate_den;
                    propertyMap.append(QStringList() << i18n("Frame rate") << QString::number(fps, 'g', 3));
                }
            }

            int scan = m_controller->int_property("meta.media.progressive");
            propertyMap.append(QStringList() << i18n("Scanning") << (scan == 1 ? i18n("Progressive") : i18n("Interlaced")));
            snprintf(property, sizeof(property), "meta.media.%d.codec.sample_aspect_ratio", vindex);
            double par = m_controller->double_property(property);
            propertyMap.append(QStringList() << i18n("Pixel aspect ratio") << QString::number(par, 'g', 3));
            propertyMap.append(QStringList() << i18n("Pixel format") << m_controller->videoCodecProperty("pix_fmt"));
            int colorspace = m_controller->videoCodecProperty("colorspace").toInt();
            propertyMap.append(QStringList() << i18n("Colorspace") << ProfilesDialog::getColorspaceDescription(colorspace));
            
            snprintf(property, sizeof(property), "meta.media.%d.codec.long_name", default_audio);
            codec = m_controller->property(property);
            if (!codec.isEmpty()) {
                propertyMap.append(QStringList() << i18n("Audio codec") << codec);
            }
            snprintf(property, sizeof(property), "meta.media.%d.codec.channels", default_audio);
            int channels = m_controller->int_property(property);
            propertyMap.append(QStringList() << i18n("Audio channels") << QString::number(channels));

            snprintf(property, sizeof(property), "meta.media.%d.codec.sample_rate", default_audio);
            int srate = m_controller->int_property(property);
            propertyMap.append(QStringList() << i18n("Audio frequency") << QString::number(srate));
        }
    }
    
    for (int i = 0; i < propertyMap.count(); i++) {
        new QTreeWidgetItem(tree, propertyMap.at(i));
    }
    tree->resizeColumnToContents(0);
}

void ClipPropertiesController::slotFillMarkers()
{
    m_markerTree->clear();
    QList <CommentedTime> markers = m_controller->commentedSnapMarkers();
    for (int count = 0; count < markers.count(); ++count) {
        QString time = m_tc.getTimecode(markers.at(count).time());
        QStringList itemtext;
        itemtext << time << markers.at(count).comment();
        QTreeWidgetItem *item = new QTreeWidgetItem(m_markerTree, itemtext);
        item->setData(0, Qt::DecorationRole, CommentedTime::markerColor(markers.at(count).markerType()));
        item->setData(0, Qt::UserRole, QVariant((int) markers.at(count).time().frames(m_tc.fps())));
    }
    m_markerTree->resizeColumnToContents(0);
}

void ClipPropertiesController::slotEditMarker()
{
    QTreeWidgetItem *item = m_markerTree->currentItem();
    int time = item->data(0, Qt::UserRole).toInt();
    emit seekToFrame(time);
    /*QList < CommentedTime > marks = m_controller->commentedSnapMarkers();
    int pos = m_markerTree->currentIndex().row();
    if (pos < 0 || pos > marks.count() - 1) return;
    QPointer<MarkerDialog> d = new MarkerDialog(m_clip, marks.at(pos), m_tc, i18n("Edit Marker"), this);
    if (d->exec() == QDialog::Accepted) {
        QList <CommentedTime> markers;
        markers << d->newMarker();
        emit addMarkers(m_clip->getId(), markers);
    }
    delete d;*/
}

