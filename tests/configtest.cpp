/*
    Checks that the generated config class clamps broken values to the ranges the settings
    dialog allows, so a hand edited config can't break the shaders (e.g. a squircle ratio of 0).

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "lightlyshaders_config.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTest>

using KWin::LightlyShadersConfig;

class ConfigTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanup();
    void defaultsAreInRange();
    void brokenValuesAreClamped();

private:
    void writeConfig(const QByteArray &contents);
    QString m_path;
};

void ConfigTest::initTestCase()
{
    // Keep the user's real configuration out of it
    QStandardPaths::setTestModeEnabled(true);
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QVERIFY(QDir().mkpath(dir));
    m_path = dir + QStringLiteral("/lightlyshaders.conf");
}

void ConfigTest::cleanup()
{
    QFile::remove(m_path);
}

void ConfigTest::writeConfig(const QByteArray &contents)
{
    QFile file(m_path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(contents);
    file.close();
    LightlyShadersConfig::self()->config()->reparseConfiguration();
    LightlyShadersConfig::self()->load();
}

void ConfigTest::defaultsAreInRange()
{
    writeConfig("");
    QCOMPARE(LightlyShadersConfig::roundness(), 5);
    QCOMPARE(LightlyShadersConfig::shadowOffset(), 2);
    QCOMPARE(LightlyShadersConfig::squircleRatio(), 5);
    QCOMPARE(LightlyShadersConfig::innerOutlineWidth(), 1);
    QCOMPARE(LightlyShadersConfig::outerOutlineWidth(), 1);
    QCOMPARE(LightlyShadersConfig::inactiveOutlines(), false);
    QCOMPARE(LightlyShadersConfig::excludedWindowClasses(), QString());
}

void ConfigTest::brokenValuesAreClamped()
{
    writeConfig("[General]\nRoundness=1000\nShadowOffset=-5\nInnerOutlineWidth=0\nOuterOutlineWidth=99\nSquircleRatio=0\n");
    QCOMPARE(LightlyShadersConfig::roundness(), 64);
    QCOMPARE(LightlyShadersConfig::shadowOffset(), 2);
    QCOMPARE(LightlyShadersConfig::innerOutlineWidth(), 1);
    QCOMPARE(LightlyShadersConfig::outerOutlineWidth(), 10);
    QCOMPARE(LightlyShadersConfig::squircleRatio(), 2);
}

QTEST_GUILESS_MAIN(ConfigTest)

#include "configtest.moc"
