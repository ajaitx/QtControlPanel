#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include <QTimer>
#include <QQueue>
#include <QFileInfo>

#include "megaind.h"
#include "rs485.h"
#include "dout.h"
#include "analog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setMinimumSize(1920, 1080);
    this->setMaximumSize(1920, 1080);
    this->setGeometry(0, 0, 1920, 1080);

#if defined(__aarch64__)
    Qt::WindowFlags flags = this->windowFlags();
    this->setWindowFlags( flags | Qt::FramelessWindowHint );
#endif
    // hide unused products
    ui->product4_rbtn->hide();
    ui->product5_rbtn->hide();

    e_run_state = eSTATE_STOPPED;

    QList<QRadioButton *> productButtons = ui->products_grpbox->findChildren<QRadioButton *>();
    for(int i = 0; i < productButtons.size(); ++i) {
        productBtngrp.addButton(productButtons[i],i);
    }
    QList<QRadioButton *> speedButtons = ui->speeds_grpbox->findChildren<QRadioButton *>();
    for(int i = 0; i < speedButtons.size(); ++i) {
       speedBtngrp.addButton(speedButtons[i],i);
    }
    connect(&productBtngrp, SIGNAL(buttonClicked(int)), this, SLOT(on_productBtngrpButtonClicked(int)) );
    connect(&speedBtngrp, SIGNAL(buttonClicked(int)), this, SLOT(on_speedBtngrpButtonClicked(int)) );
    initSystemSettings();

    ui->extruder_lcd->display(m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm);
    ui->caterpillar_lcd->display(m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm);
    ui->stepper_lcd->display(QString::number(m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].color));

    speedBtngrp.buttons().at(m_cpanel_conf_ptr->speed_idx)->setChecked(true);
    productBtngrp.buttons().at(m_cpanel_conf_ptr->product_idx)->setChecked(true);

    connect(&m_erpm_up_press_timer, SIGNAL(timeout()), this, SLOT(handleERPMIncrement()));
    connect(&m_erpm_down_press_timer, SIGNAL(timeout()), this, SLOT(handleERPMDecrement()));
    connect(&m_crpm_up_press_timer, SIGNAL(timeout()), this, SLOT(handleCRPMIncrement()));
    connect(&m_crpm_down_press_timer, SIGNAL(timeout()), this, SLOT(handleCRPMDecrement()));
    connect(&m_color_up_press_timer, SIGNAL(timeout()), this, SLOT(handleColorIncrement()));
    connect(&m_color_down_press_timer, SIGNAL(timeout()), this, SLOT(handleColorDecrement()));

#if defined(__aarch64__)
    // check and init IO board
    if ( 0 != initialize_io_board() ) {
        qDebug()<<"initialize_io_board failed!!!";
        return;
    }
#endif
    QThread::msleep(100);
    showVoltages(eERPM, false);
    showVoltages(eCRPM, false);

    return;
}

MainWindow::~MainWindow()
{
    if (m_conf_mmap_addr) {
        munmap(m_conf_mmap_addr, m_conf_file_size);
        m_conf_mmap_addr = nullptr;
    }
    QThread::sleep(1);
}

/*****************************************************************
* Initialize the IO board stacked to the rpi *
* This function finds and initialize the industrial IO board*
*****************************************************************/
/* 09/10/2023, rev-05 */
/* Called by : Mainwindow constructor*/
int MainWindow::initialize_io_board(void)
{
    // Init first io board
    m_dev = doBoardInit(0);
    if (m_dev <= 0) {
        qDebug()<<"ERROR: Failed to find and init IO board!!!";
        return -1;
    }

    u32 baud = 9600;
    u8 mode = 1, stopB = 1, parity = 0, address = 1;
    // to setup 0-10V_OUT_1
    if ( 0 != rs485Set(m_dev, mode, baud, stopB, parity, address) ) {
        qDebug()<<"ERROR: Failed to set modbus 0-10V_OUT_1!!!";
        return -1;
    }

    // to setup 0-10V_OUT_2
    address = 2;
    if ( 0 != rs485Set(m_dev, mode, baud, stopB, parity, address) ) {
        qDebug()<<"ERROR: Failed to set modbus 0-10V_OUT_2!!!";
        return -1;
    }

    // to setup OPEN_DRAIN_1
    int ch = 1;
    if ( 0 != openDrainSet(m_dev, ch) ) {
        qDebug()<<"ERROR: Failed to set OPEN_DRAIN_1";
        return -1;
    }

    return 0;
}

int MainWindow::createInitialSystemConfig(void)
{
    if (m_cpanel_conf_ptr) {
        m_cpanel_conf_ptr->product_idx= 0;
        m_cpanel_conf_ptr->speed_idx = 0;
        m_cpanel_conf_ptr->analog_factor_value = RPM_TO_VOLTAGEE_DIVIDE_FACTOR;
        m_cpanel_conf_ptr->color_factor_value = COLOR_PULSES_PER_STEP_FACTOR;
        strcpy(m_cpanel_conf_ptr->m_products[0].name, "1.25 inch");
        strcpy(m_cpanel_conf_ptr->m_products[1].name, "13/16 mm");
        strcpy(m_cpanel_conf_ptr->m_products[2].name, "2 inch");
        for (int i=0; i<MAX_PRODUCT_PARAMS_COUNT; i++) {
            for(int j=0; j<MAX_SPEED_PARAMS_COUNT; j++) {
                m_cpanel_conf_ptr->m_products[i].params[j].erpm = ((i+1)*(j+1)+2)*100;
                m_cpanel_conf_ptr->m_products[i].params[j].crpm = ((i+1)*(j+1)+2)*50;
                m_cpanel_conf_ptr->m_products[i].params[j].color = (float)4.6+((i+1)*(j+1)+1.0);
            }
        }
    } else {
        qDebug()<<"Failed to create initial sstem config!!!";
        return -1;
    }

    return 0;
}

int MainWindow::initSystemSettings(void)
{
    bool init_default_sys_config = false;

    m_cpanel_config_file.setFileName(SYSTEM_SETTINGS_FILE);
    QFileInfo check_file(SYSTEM_SETTINGS_FILE);
    if ( ! check_file.exists() ) {
        // create control panel config file

        if (!m_cpanel_config_file.open(QIODevice::WriteOnly)) {
            qDebug() << "Failed to create and open the controlpanel settings file";
            return -1;
        }
        m_cpanel_config_file.resize( sizeof(struct ControlPanelConfig) + 32);
        QDataStream out(&m_cpanel_config_file);
        QByteArray data(m_cpanel_config_file.size(), '\0');
        out.writeRawData(data.constData(), data.size());
        m_cpanel_config_file.close();
        qDebug() << "New controlpanel settings file created.";
        init_default_sys_config = true;
    }

    if (!m_cpanel_config_file.open(QIODevice::ReadWrite)) {
        qDebug() << "Failed to open the controlpanel settings file";
        return -1;
    }
    m_conf_file_size = m_cpanel_config_file.size();
    // Map the file into memory
    int fd = m_cpanel_config_file.handle();
    m_conf_mmap_addr = mmap(nullptr, m_cpanel_config_file.size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (m_conf_mmap_addr == MAP_FAILED) {
        m_cpanel_config_file.close();
        qDebug() << "controlpanel settings mapping failed!!!";
        return -1;
    }

    m_cpanel_conf_ptr = static_cast<struct ControlPanelConfig *>(m_conf_mmap_addr);

    if (init_default_sys_config) {
        createInitialSystemConfig();
    }

    return 0;
}

void MainWindow::on_extruder_up_btn_clicked()
{
    int erpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm;
    if ( erpm < (m_cpanel_conf_ptr->analog_factor_value * ANALOG_VOLTAGE_MAX) ) {
        ui->extruder_lcd->display(++erpm);
        m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm = erpm;
    }
    showVoltages(eERPM);
}

void MainWindow::on_extruder_down_btn_clicked()
{
    int erpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm;
    if ( erpm > EXTRUDER_RPM_MIN ) {
        ui->extruder_lcd->display(--erpm);
        m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm = erpm;
    }
    showVoltages(eERPM);
}

void MainWindow::on_caterpillar_up_btn_clicked()
{
    int crpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm;
    if (crpm < (m_cpanel_conf_ptr->analog_factor_value * ANALOG_VOLTAGE_MAX)) {
        ui->caterpillar_lcd->display(++crpm);
        m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm = crpm;
    }
    showVoltages(eCRPM);
}

void MainWindow::on_caterpillar_down_btn_clicked()
{
    int crpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm;
    if (crpm > EXTRUDER_RPM_MIN) {
        ui->caterpillar_lcd->display(--crpm);
        m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm = crpm;
    }
    showVoltages(eCRPM);
}

void MainWindow::on_stepper_up_btn_clicked()
{
    float color = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].color;
    color = (float)(color + 0.1);
    ui->stepper_lcd->display(QString::number(color));
    m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].color = color;
}

void MainWindow::on_stepper_down_btn_clicked()
{
    float color = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].color;
    color = (float)(color - 0.1);
    ui->stepper_lcd->display(QString::number(color));
    m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].color = color;
}

void MainWindow::on_settings_btn_clicked()
{
    ui->ss_products_cbox->setCurrentIndex(m_cpanel_conf_ptr->product_idx);
    ui->ss_speeds_cbox->setCurrentIndex(m_cpanel_conf_ptr->speed_idx);
    // disable edit
    ui->ss_maxerpm_ledit->setText(QString::number(m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm));
    ui->ss_crpmfactor_ledit->setText(QString::number(m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm));
    ui->ss_colorfactor_ledit->setText(QString::number(m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].color));
    ui->ss_maxerpm_ledit->setReadOnly(true);
    ui->ss_crpmfactor_ledit->setReadOnly(true);
    ui->ss_colorfactor_ledit->setReadOnly(true);
    ui->stackedWidget->setCurrentIndex(eSETTINGS_SCREEN);
}

void MainWindow::on_ss_back_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(eRUN_SCREEN);
}

void MainWindow::on_productBtngrpButtonClicked(int product_idx)
{
    m_cpanel_conf_ptr->product_idx = product_idx;
    m_cpanel_conf_ptr->speed_idx = speedBtngrp.checkedId();

    ui->extruder_lcd->display(m_cpanel_conf_ptr->m_products[product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm);
    ui->caterpillar_lcd->display(m_cpanel_conf_ptr->m_products[product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm);
    ui->stepper_lcd->display(QString::number(m_cpanel_conf_ptr->m_products[product_idx].params[m_cpanel_conf_ptr->speed_idx].color));
    showVoltages(eERPM);
    showVoltages(eCRPM);
}

void MainWindow::on_speedBtngrpButtonClicked(int speed_idx)
{
    m_cpanel_conf_ptr->speed_idx = speed_idx;
    m_cpanel_conf_ptr->product_idx = productBtngrp.checkedId();

    ui->extruder_lcd->display(m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[speed_idx].erpm);
    ui->caterpillar_lcd->display(m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[speed_idx].crpm);
    ui->stepper_lcd->display(QString::number(m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[speed_idx].color));
    showVoltages(eERPM);
    showVoltages(eCRPM);
}

void MainWindow::showVoltages(enum ParameterTypes type, bool set_voltage)
{
    float volt = 0.0;

    if ( type == eERPM ) {
        int erpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm;
        volt = (float)((float)erpm / (float)(m_cpanel_conf_ptr->analog_factor_value));
        QString formattedValue = QString::number(volt, 'f', 2);
        if (set_voltage) {
            if (0 == setStartVoltages(eERPM)) {
                ui->erpm_volt->setStyleSheet("color: green; border: 3px solid white; border-radius: 7px;");
            } else {
                ui->erpm_volt->setStyleSheet("color: rgb(246, 97, 81); border: 3px solid white; border-radius: 7px;");
            }
        }
        ui->erpm_volt->setText(formattedValue + "V");
    } else {
        int crpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm;
        volt = (float)((float)crpm / (float)((m_cpanel_conf_ptr->analog_factor_value)));
        QString formattedValue = QString::number(volt, 'f', 2);
        if (set_voltage) {
            if (0 == setStartVoltages(eCRPM)) {
                ui->crpm_volt->setStyleSheet("color: green; border: 3px solid white; border-radius: 7px;");
            } else {
                ui->crpm_volt->setStyleSheet("color: rgb(246, 97, 81); border: 3px solid white; border-radius: 7px;");
            }
        }
        ui->crpm_volt->setText(formattedValue + "V");
    }
}

/*****************************************************************
* Set the analog output voltage for ERPM and CRPM on start condition*
* It set the voltage for channel 1 & 2 based on the extruder and caterpillar RPMs in the UI *
*****************************************************************/
/* 09/10/2023, rev-05 */
/* Called by : on_run_btn_clicked*/
int MainWindow::setStartVoltages(enum ParameterTypes type)
{
    float volt = 0.0;

#if defined(__aarch64__)
    if (m_dev == -1 ) {
        qDebug()<<"ERROR: the IO board not initialized yet!!!";
        return -1;
    }
#endif
    if ( (type == eERPM) || (type == eALL_PARAMS) ) {
        // For erpm
        int erpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm;
        volt = (float)((float)erpm / (float)(m_cpanel_conf_ptr->analog_factor_value));
#if defined(__aarch64__)
        // Channel 1 for erpm
        if ( 0 != analogOutVoltageWrite(m_dev, 1, volt) ) {
            qDebug()<<"ERROR: failed to set voltage for erpm!!!";
            return -1;
        }
#endif
    }

    if ( (type == eCRPM) || (type == eALL_PARAMS) ) {
        // For crpm
        int crpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm;
        volt = (float)((float)crpm / (float)((m_cpanel_conf_ptr->analog_factor_value)));
#if defined(__aarch64__)
        // Channel 2 for crpm
        if ( 0 != analogOutVoltageWrite(m_dev, 2, volt) ) {
            qDebug()<<"ERROR: failed to set voltage for crpm!!!";
            return -1;
        }
#endif
    }

    return 0;
}

/*****************************************************************
* Set the analog output voltage for ERPM and CRPM on stop condition*
* The extruder and caterpillar voltages set to 0.0 for the stop *
*****************************************************************/
/* 09/10/2023, rev-05 */
/* Called by : on__clicked*/
int MainWindow::setStopVoltages(void)
{
    float volt = 0.0;

#if defined(__aarch64__)
    if (m_dev == -1 ) {
        qDebug()<<"ERROR: the IO board not initialized yet!!!";
        return -1;
    }

    // For erpm
    analogOutVoltageWrite(m_dev, 1, volt); // Channel 1 for erpm

    // For crpm
    analogOutVoltageWrite(m_dev, 2, volt); // Channel 2 for crpm
#endif
    ui->erpm_volt->setStyleSheet("color: rgb(246, 97, 81); border: 3px solid white; border-radius: 7px;");
    ui->crpm_volt->setStyleSheet("color: rgb(246, 97, 81); border: 3px solid white; border-radius: 7px;");

    return 0;
}

void MainWindow::on_run_btn_clicked()
{
    if ( ui->run_btn->text().compare("START") == 0 ) {
        if ( 0 == setStartVoltages(eALL_PARAMS) ) {
            e_run_state = eSTATE_STARTED;
            ui->products_grpbox->setEnabled(false);
            ui->settings_btn->setVisible(false);
            ui->run_btn->setText("STOP");
            ui->run_btn->setStyleSheet("background-color: rgb(246, 97, 81); border-radius: 30px;");
            showVoltages(eERPM);
            showVoltages(eCRPM);
        }
    } else {
        if ( 0 == setStopVoltages() ) {
            e_run_state = eSTATE_STOPPED;
            ui->products_grpbox->setEnabled(true);
            ui->settings_btn->setVisible(true);
            ui->run_btn->setText("START");
            ui->run_btn->setStyleSheet("background-color: rgb(38, 162, 105); border-radius: 30px;");
        }
    }
}

void MainWindow::on_ss_params_editsave_btn_clicked()
{
    if ( !ss_profile_edit ) {
        ss_profile_edit = true;
        ui->ss_maxerpm_ledit->setReadOnly(false);
        ui->ss_crpmfactor_ledit->setReadOnly(false);
        ui->ss_colorfactor_ledit->setReadOnly(false);
        ui->ss_params_editsave_btn->setStyleSheet("image: url(:/icons/save.svg); border: 1px solid #4fa08b; background-color: #222b2e;");
        ui->ss_maxerpm_ledit->setModified(true);
    } else {
        ss_profile_edit = false;
        int speep_idx = ui->ss_speeds_cbox->currentIndex();
        int product_idx = ui->ss_products_cbox->currentIndex();
        m_cpanel_conf_ptr->m_products[product_idx].params[speep_idx].erpm = ui->ss_maxerpm_ledit->text().toInt();
        m_cpanel_conf_ptr->m_products[product_idx].params[speep_idx].crpm = ui->ss_crpmfactor_ledit->text().toInt();
        m_cpanel_conf_ptr->m_products[product_idx].params[speep_idx].color = ui->ss_colorfactor_ledit->text().toFloat();

        ui->ss_maxerpm_ledit->setReadOnly(true);
        ui->ss_crpmfactor_ledit->setReadOnly(true);
        ui->ss_colorfactor_ledit->setReadOnly(true);
        ui->ss_params_editsave_btn->setStyleSheet("image: url(:/icons/edit.svg); border: 1px solid #4fa08b; background-color: #000000;");
        ui->ss_maxerpm_ledit->setStyleSheet("border: 1px solid #4fa08b; background-color: #222b2e; color: #d3dae3;");
        ui->ss_crpmfactor_ledit->setStyleSheet("border: 1px solid #4fa08b; background-color: #222b2e; color: #d3dae3;");
        ui->ss_colorfactor_ledit->setStyleSheet("border: 1px solid #4fa08b; background-color: #222b2e; color: #d3dae3;");
    }
}

void MainWindow::on_ss_maxerpm_ledit_textEdited(const QString &text)
{
    int product_idx = ui->ss_products_cbox->currentIndex();
    int speep_idx = ui->ss_speeds_cbox->currentIndex();
    m_cpanel_conf_ptr->m_products[product_idx].params[speep_idx].erpm = text.toInt();
    ui->ss_maxerpm_ledit->setStyleSheet("border: 1px solid red;");
    ui->ss_params_editsave_btn->setStyleSheet("image: url(:/icons/save.svg); border: 1px solid red; background-color: #222b2e;");
}

void MainWindow::on_ss_crpmfactor_ledit_textEdited(const QString &text)
{
    int product_idx = ui->ss_products_cbox->currentIndex();
    int speep_idx = ui->ss_speeds_cbox->currentIndex();
    m_cpanel_conf_ptr->m_products[product_idx].params[speep_idx].crpm = text.toInt();
    ui->ss_crpmfactor_ledit->setStyleSheet("border: 1px solid red;");
    ui->ss_params_editsave_btn->setStyleSheet("image: url(:/icons/save.svg); border: 1px solid red; background-color: #222b2e;");
}

void MainWindow::on_ss_colorfactor_ledit_textEdited(const QString &text)
{    
    int product_idx = ui->ss_products_cbox->currentIndex();
    int speep_idx = ui->ss_speeds_cbox->currentIndex();
    m_cpanel_conf_ptr->m_products[product_idx].params[speep_idx].color = text.toInt();
    ui->ss_colorfactor_ledit->setStyleSheet("border: 1px solid red;");
    ui->ss_params_editsave_btn->setStyleSheet("image: url(:/icons/save.svg); border: 1px solid red; background-color: #222b2e;");
}

void MainWindow::on_ss_product_edit_btn_clicked()
{
    ui->ins_input_text_ledit->setText(ui->ss_products_cbox->currentText());
    ui->ins_item_name_lbl->setText(ui->ss_products_lbl->text());
    ui->stackedWidget->setCurrentIndex(eINPUT_TEXT_SCREEN);
}

void MainWindow::on_ins_input_text_ledit_returnPressed()
{
    if ( ! ui->ins_item_name_lbl->text().compare(ui->ss_products_lbl->text()) ) {
        ui->ss_products_cbox->setItemText(ui->ss_products_cbox->currentIndex(), ui->ins_input_text_ledit->text());
        productBtngrp.buttons().at(ui->ss_products_cbox->currentIndex())->setText(ui->ins_input_text_ledit->text());
    } else if ( ! ui->ins_item_name_lbl->text().compare(ui->ss_speeds_lbl->text()) ) {
        ui->ss_speeds_cbox->setItemText(ui->ss_speeds_cbox->currentIndex(), ui->ins_input_text_ledit->text());
        speedBtngrp.buttons().at(ui->ss_speeds_cbox->currentIndex())->setText(ui->ins_input_text_ledit->text());
    }
    ui->ins_input_text_ledit->setStyleSheet("border: 1px solid #4fa08b; background-color: #222b2e; color: #d3dae3;");
    ui->stackedWidget->setCurrentIndex(eSETTINGS_SCREEN);
}

void MainWindow::on_ins_back_btn_clicked()
{
    ui->ss_products_cbox->setItemText(ui->ss_products_cbox->currentIndex(), ui->ins_input_text_ledit->text());
    ui->stackedWidget->setCurrentIndex(eSETTINGS_SCREEN);
}

void MainWindow::on_ins_input_text_ledit_textEdited(const QString &text)
{
    (void)text;
    ui->ins_input_text_ledit->setStyleSheet("border: 1px solid red; background-color: #222b2e; color: #d3dae3;");
    ui->ins_input_save_btn->setStyleSheet("image: url(:/icons/save.svg); border: 1px solid red;");
}

void MainWindow::on_ss_speed_edit_btn_clicked()
{
    ui->ins_input_text_ledit->setText(ui->ss_speeds_cbox->currentText());
    ui->ins_item_name_lbl->setText(ui->ss_speeds_lbl->text());
    ui->stackedWidget->setCurrentIndex(eINPUT_TEXT_SCREEN);
}

void MainWindow::on_ss_products_cbox_currentIndexChanged(int product_idx)
{
    if (product_idx > MAX_PRODUCT_PARAMS_COUNT) {
        return;
    }
    int speed_idx = ui->ss_speeds_cbox->currentIndex();
    ui->ss_maxerpm_ledit->setText(QString::number(m_cpanel_conf_ptr->m_products[product_idx].params[speed_idx].erpm));
    ui->ss_crpmfactor_ledit->setText(QString::number(m_cpanel_conf_ptr->m_products[product_idx].params[speed_idx].crpm));
    ui->ss_colorfactor_ledit->setText(QString::number(m_cpanel_conf_ptr->m_products[product_idx].params[speed_idx].color));
}

void MainWindow::on_ss_speeds_cbox_currentIndexChanged(int speed_idx)
{
    if (speed_idx > MAX_SPEED_PARAMS_COUNT) {
        return;
    }
    int product_idx = ui->ss_products_cbox->currentIndex();
    ui->ss_maxerpm_ledit->setText(QString::number(m_cpanel_conf_ptr->m_products[product_idx].params[speed_idx].erpm));
    ui->ss_crpmfactor_ledit->setText(QString::number(m_cpanel_conf_ptr->m_products[product_idx].params[speed_idx].crpm));
    ui->ss_colorfactor_ledit->setText(QString::number(m_cpanel_conf_ptr->m_products[product_idx].params[speed_idx].color));
}

void MainWindow::on_ins_input_save_btn_clicked()
{
    if ( ! ui->ins_item_name_lbl->text().compare(ui->ss_products_lbl->text()) ) {
        ui->ss_products_cbox->setItemText(ui->ss_products_cbox->currentIndex(), ui->ins_input_text_ledit->text());
        productBtngrp.buttons().at(ui->ss_products_cbox->currentIndex())->setText(ui->ins_input_text_ledit->text());
    } else if ( ! ui->ins_item_name_lbl->text().compare(ui->ss_speeds_lbl->text()) ) {
        ui->ss_speeds_cbox->setItemText(ui->ss_speeds_cbox->currentIndex(), ui->ins_input_text_ledit->text());
        speedBtngrp.buttons().at(ui->ss_speeds_cbox->currentIndex())->setText(ui->ins_input_text_ledit->text());
    }
    ui->ins_input_text_ledit->setStyleSheet("border: 1px solid #4fa08b; background-color: #222b2e; color: #d3dae3;");
    ui->ins_input_save_btn->setStyleSheet("image: url(:/icons/save.svg); border: 1px solid #4fa08b;");
    ui->stackedWidget->setCurrentIndex(eSETTINGS_SCREEN);
}

void MainWindow::handleERPMIncrement(void)
{
    int erpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm;
    erpm += m_long_press_factor * 10;
    if ( m_long_press_factor < 10 )
        ++m_long_press_factor;
    if ( erpm > (m_cpanel_conf_ptr->analog_factor_value * ANALOG_VOLTAGE_MAX) )
        erpm = (m_cpanel_conf_ptr->analog_factor_value * ANALOG_VOLTAGE_MAX);
    m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm = erpm;
    ui->extruder_lcd->display(erpm);
    showVoltages(eERPM);
}

void MainWindow::handleERPMDecrement(void)
{
    int erpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm;
    erpm -= m_long_press_factor * 10;
    if ( m_long_press_factor < 10 )
        ++m_long_press_factor;
    if ( erpm < EXTRUDER_RPM_MIN )
        erpm = EXTRUDER_RPM_MIN;
    m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].erpm = erpm;
    ui->extruder_lcd->display(erpm);
    showVoltages(eERPM);
}

void MainWindow::handleCRPMIncrement(void)
{
    int crpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm;
    crpm += m_long_press_factor * 10;
    if ( m_long_press_factor < 10 )
        ++m_long_press_factor;
    if ( crpm > (m_cpanel_conf_ptr->analog_factor_value * ANALOG_VOLTAGE_MAX) )
        crpm = (m_cpanel_conf_ptr->analog_factor_value * ANALOG_VOLTAGE_MAX);
    m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm = crpm;
    ui->caterpillar_lcd->display(crpm);
    showVoltages(eCRPM);
}

void MainWindow::handleCRPMDecrement(void)
{
    int crpm = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm;
    crpm -= m_long_press_factor * 10;
    if ( m_long_press_factor < 10 )
        ++m_long_press_factor;
    if ( crpm < CATERPILLAR_RPM_MIN )
        crpm = CATERPILLAR_RPM_MIN;
    m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].crpm = crpm;
    ui->caterpillar_lcd->display(crpm);
    showVoltages(eCRPM);
}

void MainWindow::handleColorIncrement(void)
{
    float color = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].color;
    color += (float)m_long_press_factor * 0.1;
    if (m_long_press_factor < 10)
        ++m_long_press_factor;
    m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].color = color;
    ui->stepper_lcd->display(QString::number(color));
}

void MainWindow::handleColorDecrement(void)
{
    float color = m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].color;
    color -= (float)m_long_press_factor * 0.1;
    if (m_long_press_factor < 10)
        ++m_long_press_factor;
    m_cpanel_conf_ptr->m_products[m_cpanel_conf_ptr->product_idx].params[m_cpanel_conf_ptr->speed_idx].color = color;
    ui->stepper_lcd->display(QString::number(color));
}

void MainWindow::on_extruder_up_btn_pressed()
{
    m_long_press_factor = 1;
    ui->extruder_up_btn->setStyleSheet("image: url(:/icons/up.svg); border: 5px solid rgb(246, 97, 81); border-radius: 50px;");
    m_erpm_up_press_timer.start(1000); // 1 sec
}

void MainWindow::on_extruder_up_btn_released()
{
    ui->extruder_up_btn->setStyleSheet("image: url(:/icons/up.svg)");
    if (m_erpm_up_press_timer.isActive()) {
        m_erpm_up_press_timer.stop();
    }
}

void MainWindow::on_extruder_down_btn_pressed()
{
    m_long_press_factor = 1;
    ui->extruder_down_btn->setStyleSheet("image: url(:/icons/down.svg); border: 5px solid rgb(246, 97, 81); border-radius: 50px;");
    m_erpm_down_press_timer.start(1000); // 1 sec
}

void MainWindow::on_extruder_down_btn_released()
{
    ui->extruder_down_btn->setStyleSheet("image: url(:/icons/down.svg)");
    if (m_erpm_down_press_timer.isActive()) {
        m_erpm_down_press_timer.stop();
    }
}

void MainWindow::on_caterpillar_up_btn_pressed()
{
    m_long_press_factor = 1;
    ui->caterpillar_up_btn->setStyleSheet("image: url(:/icons/up.svg); border: 5px solid rgb(246, 97, 81); border-radius: 50px;");
    m_crpm_up_press_timer.start(1000); // 1 sec
}

void MainWindow::on_caterpillar_up_btn_released()
{
    ui->caterpillar_up_btn->setStyleSheet("image: url(:/icons/up.svg)");
    if (m_crpm_up_press_timer.isActive()) {
        m_crpm_up_press_timer.stop();
    }
}

void MainWindow::on_caterpillar_down_btn_pressed()
{
    m_long_press_factor = 1;
    ui->caterpillar_down_btn->setStyleSheet("image: url(:/icons/down.svg); border: 5px solid rgb(246, 97, 81); border-radius: 50px;");
    m_crpm_down_press_timer.start(1000); // 1 sec
}

void MainWindow::on_caterpillar_down_btn_released()
{
    ui->caterpillar_down_btn->setStyleSheet("image: url(:/icons/down.svg)");
    if (m_crpm_down_press_timer.isActive()) {
        m_crpm_down_press_timer.stop();
    }
}

void MainWindow::on_stepper_up_btn_pressed()
{
    m_long_press_factor = 1;
    ui->stepper_up_btn->setStyleSheet("image: url(:/icons/up.svg); border: 5px solid rgb(246, 97, 81); border-radius: 50px;");
    m_color_up_press_timer.start(1000); // 1 sec
}

void MainWindow::on_stepper_up_btn_released()
{
    ui->stepper_up_btn->setStyleSheet("image: url(:/icons/up.svg)");
    if (m_color_up_press_timer.isActive()) {
        m_color_up_press_timer.stop();
    }
}

void MainWindow::on_stepper_down_btn_pressed()
{
    m_long_press_factor = 1;
    ui->stepper_down_btn->setStyleSheet("image: url(:/icons/down.svg); border: 5px solid rgb(246, 97, 81); border-radius: 50px;");
    m_color_down_press_timer.start(1000); // 1 sec
}

void MainWindow::on_stepper_down_btn_released()
{
    ui->stepper_down_btn->setStyleSheet("image: url(:/icons/down.svg)");
    if (m_color_down_press_timer.isActive()) {
        m_color_down_press_timer.stop();
    }
}

void MainWindow::on_ss_factor_edit_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(eFACTORS_EDIT_SCREEN);
    ui->fes_analogfactor_edit->setText(QString::number(m_cpanel_conf_ptr->analog_factor_value));
    ui->fes_colorfactor_edit->setText(QString::number(m_cpanel_conf_ptr->color_factor_value));
    ui->fes_analogfactor_edit->setReadOnly(true);
    ui->fes_colorfactor_edit->setReadOnly(true);
}

void MainWindow::on_fes_back_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(eSETTINGS_SCREEN);
}

void MainWindow::on_fes_analogfactor_editsave_btn_clicked()
{
    if ( !fes_analog_factor_edit ) {
        fes_analog_factor_edit = true;
        ui->fes_analogfactor_edit->setReadOnly(false);
        ui->fes_analogfactor_editsave_btn->setStyleSheet("image: url(:/icons/save.svg); border: 1px solid #4fa08b; background-color: #222b2e;");
    } else {
        fes_analog_factor_edit = false;
        m_cpanel_conf_ptr->analog_factor_value = ui->fes_analogfactor_edit->text().toFloat();
        ui->fes_analogfactor_edit->setReadOnly(true);
        ui->fes_analogfactor_editsave_btn->setStyleSheet("image: url(:/icons/edit.svg); border: 1px solid #4fa08b; background-color: #000000;");
        ui->fes_analogfactor_edit->setStyleSheet("border: 1px solid #4fa08b; background-color: #222b2e; color: #d3dae3;");
    }
}

void MainWindow::on_fes_analogfactor_edit_textEdited(const QString &text)
{
    m_cpanel_conf_ptr->analog_factor_value = text.toInt();
    ui->fes_analogfactor_edit->setStyleSheet("border: 1px solid red;");
    ui->fes_analogfactor_editsave_btn->setStyleSheet("image: url(:/icons/save.svg); border: 1px solid red; background-color: #222b2e;");
}

void MainWindow::on_fes_colorfactor_editsave_btn_clicked()
{
    if ( !fes_color_factor_edit ) {
        fes_color_factor_edit = true;
        ui->fes_colorfactor_edit->setReadOnly(false);
        ui->fes_colorfactor_editsave_btn->setStyleSheet("image: url(:/icons/save.svg); border: 1px solid #4fa08b; background-color: #222b2e;");
    } else {
        fes_color_factor_edit = false;
        m_cpanel_conf_ptr->color_factor_value = ui->fes_colorfactor_edit->text().toFloat();
        ui->fes_colorfactor_edit->setReadOnly(true);
        ui->fes_colorfactor_editsave_btn->setStyleSheet("image: url(:/icons/edit.svg); border: 1px solid #4fa08b; background-color: #000000;");
        ui->fes_colorfactor_edit->setStyleSheet("border: 1px solid #4fa08b; background-color: #222b2e; color: #d3dae3;");
    }
}

void MainWindow::on_fes_colorfactor_edit_textEdited(const QString &text)
{
    m_cpanel_conf_ptr->color_factor_value = text.toInt();
    ui->fes_colorfactor_edit->setStyleSheet("border: 1px solid red;");
    ui->fes_colorfactor_editsave_btn->setStyleSheet("image: url(:/icons/save.svg); border: 1px solid red; background-color: #222b2e;");
}
