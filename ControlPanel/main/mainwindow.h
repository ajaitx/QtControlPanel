#pragma once

#include <QMainWindow>
#include "common.h"
#include <QTimer>
#include <QLabel>
#include <QElapsedTimer>
#include <QButtonGroup>
#include <QFile>
#include <sys/mman.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int createInitialSystemConfig(void);
    int initSystemSettings(void);
    int initialize_io_board(void);
    int setStartVoltages(ParameterTypes type);
    int setStopVoltages(void);    
    void showVoltages(ParameterTypes type, bool set_voltage=true);

public slots:

protected:

private slots:
    void on_extruder_up_btn_clicked();
    void on_extruder_down_btn_clicked();
    void on_caterpillar_up_btn_clicked();
    void on_caterpillar_down_btn_clicked();
    void on_stepper_up_btn_clicked();
    void on_stepper_down_btn_clicked();
    void on_settings_btn_clicked();
    void on_ss_back_btn_clicked();
    void on_run_btn_clicked();
    void on_ss_params_editsave_btn_clicked();
    void on_ss_products_cbox_currentIndexChanged(int index);
    void on_ss_maxerpm_ledit_textEdited(const QString &arg1);
    void on_ss_crpmfactor_ledit_textEdited(const QString &arg1);
    void on_ss_colorfactor_ledit_textEdited(const QString &arg1);
    void on_ss_product_edit_btn_clicked();
    void on_ins_input_text_ledit_returnPressed();
    void on_ins_back_btn_clicked();
    void on_ins_input_text_ledit_textEdited(const QString &arg1);
    void on_ss_speed_edit_btn_clicked();
    void on_ss_speeds_cbox_currentIndexChanged(int index);
    void on_ins_input_save_btn_clicked();
    void on_extruder_up_btn_pressed();
    void on_extruder_down_btn_released();
    void on_extruder_up_btn_released();
    void on_extruder_down_btn_pressed();
    void on_caterpillar_up_btn_pressed();
    void on_caterpillar_down_btn_released();
    void on_stepper_up_btn_pressed();
    void on_stepper_up_btn_released();
    void on_stepper_down_btn_pressed();
    void on_stepper_down_btn_released();
    void on_caterpillar_up_btn_released();
    void on_caterpillar_down_btn_pressed();
    void on_ss_factor_edit_btn_clicked();
    void on_fes_back_btn_clicked();
    void on_fes_analogfactor_editsave_btn_clicked();
    void on_fes_analogfactor_edit_textEdited(const QString &text);

    void on_fes_colorfactor_editsave_btn_clicked();

    void on_fes_colorfactor_edit_textEdited(const QString &arg1);

public slots:
    void on_productBtngrpButtonClicked(int);
    void on_speedBtngrpButtonClicked(int);
    void handleERPMIncrement();
    void handleERPMDecrement();
    void handleCRPMIncrement();
    void handleCRPMDecrement();
    void handleColorIncrement();
    void handleColorDecrement();

private:
    Ui::MainWindow *ui;
    int m_dev = -1;
    enum RunStates e_run_state;
    bool ss_profile_edit = false;
    bool fes_analog_factor_edit = false;
    bool fes_color_factor_edit = false;
    QButtonGroup productBtngrp;
    QButtonGroup speedBtngrp;
    QFile m_cpanel_config_file;
    int m_conf_file_size = 0;
    void *m_conf_mmap_addr = nullptr;
    struct ControlPanelConfig *m_cpanel_conf_ptr = nullptr;
    int m_long_press_factor;
    QTimer m_erpm_up_press_timer;
    QTimer m_erpm_down_press_timer;
    QTimer m_crpm_up_press_timer;
    QTimer m_crpm_down_press_timer;
    QTimer m_color_up_press_timer;
    QTimer m_color_down_press_timer;
};
