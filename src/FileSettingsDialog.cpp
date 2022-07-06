#include "FileSettingsDialog.h"

#include <utility>

namespace br_ui
{

    FileSettingsDialog::FileSettingsDialog(QWidget* parent, SettingFor settingFor,
                                           brlib::ManualParseSettings settings):
        QDialog(parent),
        m_settingsFor(settingFor),
        m_settings(std::move(settings))
    {

        setupUi(this);
        populateDateFormats(); /* Combobox options array defined in brlib for manual
                          configuration. */
        populateDelims();
        connectSignals();
        setupColIndices();
    }

    void FileSettingsDialog::populateDateFormats()
    {
        if (!cmbDateFmt->count())
        {
            for (int i = 0; i < std::size(brlib::dateFormats); ++i)
            {
                cmbDateFmt->insertItem(i, QString(brlib::dateFormats[i].label.data()),
                                       Qt::DisplayRole);
            }
        }
    }

    void FileSettingsDialog::populateDelims()
    {
        if (!cmbDelim->count())
        {
            for (int i = 0; i < std::size(brlib::delims); ++i)
            {
                cmbDelim->insertItem(i, QString(brlib::delims[i].label.data()));
            }
        }
    }

    void FileSettingsDialog::setupColIndices()
    {
        /* do not change the order of these assignments and func calls. */
        colCombos[0] = cmbDateCol;
        colCombos[1] = cmbNarrCol;
        colCombos[2] = cmbDebitCol;
        colCombos[3] = cmbCreditCol;
        colCombos[4] = cmbBalanceCol;
        populateColIndices();
        populateColIndicesDefaults();
    }

    void FileSettingsDialog::populateColIndices()
    {
        QStringList indices = {"1", "2", "3", "4", "5"};
        for (auto& c : colCombos)
        {
            if (c)
            {
                c->addItems(indices);
            }
        }
    }

    void FileSettingsDialog::populateColIndicesDefaults()
    {
        for (int i = 0; i < std::size(colCombos); ++i)
        {
            auto& combo = colCombos[i];
            if (combo)
            {
                combo->setCurrentIndex(i);
                connect(combo, &QComboBox::currentIndexChanged, this,
                        [&](int idx) {
                            colIndexChanged(idx, static_cast<Cols>(idx));
                        });
            }
        }
    };

    void FileSettingsDialog::dateFormatChanged(int index)
    {
        if (index < 0 || index >= std::size(brlib::dateFormats))
        {
            return;
        }
        m_settings.dateFormat = brlib::dateFormats[index];
    }

    void FileSettingsDialog::delimChanged(int index)
    {
        if (index < 0 || index >= std::size(brlib::delims))
        {
            return;
        }
        m_settings.delimChar = brlib::delims[index].value;
    }

    void FileSettingsDialog::connectSignals()
    {

        connect(txtFirstRow, &QSpinBox::valueChanged, this,
                [&](int val) {
                    m_settings.firstRowAt = val;
                });
        connect(cmbDateFmt, &QComboBox::currentIndexChanged, this,
                &FileSettingsDialog::dateFormatChanged);
        connect(cmbDelim, &QComboBox::currentIndexChanged, this,
                &FileSettingsDialog::delimChanged);
        connect(txtNumCols, &QSpinBox::valueChanged, this,
                [&](int value) {
                    m_settings.numCols = value;
                });
        connect(chkSingleAmountCol, &QCheckBox::stateChanged, this,
                &FileSettingsDialog::singleAmountColChanged);
    }

    void FileSettingsDialog::colIndexChanged(int idx, Cols col)
    {
        m_settings.colIndices[col] = idx;
    }

    void FileSettingsDialog::singleAmountColChanged(bool state)
    {
        auto disconnectSignals = [&]() {
            cmbDebitCol->disconnect(this);
            cmbCreditCol->disconnect(this);
        };
        auto reconnectSignals = [&]() {
            connect(cmbDebitCol, &QComboBox::currentIndexChanged, this,
                    [&](int value) {
                        colIndexChanged(value, Cols::TransactionType);
                    });
            connect(cmbCreditCol, &QComboBox::currentIndexChanged, this,
                    [&](int value) {
                        colIndexChanged(value, Cols::Amount);
                    });
        };
        if (state)
        {
            lblDebitCol->setText("Transaction Type");
            lblCreditCol->setText("Amount");
            m_settings.colIndices[Cols::Debit] = -1;
            m_settings.colIndices[Cols::Credit] = -1;
            disconnectSignals();
            m_settings.colIndices[Cols::TransactionType] = cmbDebitCol->currentIndex();
            m_settings.colIndices[Cols::Amount] = cmbCreditCol->currentIndex();
            reconnectSignals();
        }
        else
        {
            lblDebitCol->setText("Debit");
            lblCreditCol->setText("Credit");
            m_settings.colIndices[Cols::TransactionType] = -1;
            m_settings.colIndices[Cols::Amount] = -1;
            disconnectSignals();
            m_settings.colIndices[Cols::Debit] = cmbDebitCol->currentIndex();
            m_settings.colIndices[Cols::Credit] = cmbCreditCol->currentIndex();
            reconnectSignals();
        }
    }
} // namespace br_ui
