#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //note:数据库及参数初始化
    this->m_db = QSqlDatabase::addDatabase("QSQLITE");
    this->m_db.setDatabaseName("ildb.db");
    this->m_query = new QSqlQuery(m_db);
    this->m_query_ref = new QSqlQuery(m_db);

    if(m_db.open())
        this->ui->statusBar->showMessage("Successful connection.",3000);
    else
        qDebug() << "Error: can not open database: " << m_db.lastError();

    //note:设置completer
    this->setCompleter();

    //TODO:加入Thermodynamics的参数，以及其他参数
    this->ui->ckBThermo->setEnabled(false);

    //note: status bar 表明著作权及所属
    QLabel *statusLabel = new QLabel();
    statusLabel->setText("© <a style=\"color: black\" href=\"https://jonahliu.cf/\">Jonah Liu</a>'s academic kits, proudly using Qt5");
    statusLabel->setOpenExternalLinks(true);
    this->ui->statusBar->addPermanentWidget(statusLabel);
    this->ui->statusBar->setStyleSheet(QString("QStatusBar::item{border: 0px}"));

    //note: 提示文本
    this->ui->textBrowser->setText("<center><br/><h2>IL information query tool</h2>"
                                   "<p>Made by <a style=\"color: black\" href=\"https://jonahliu.cf/\">Jonah Liu</a></p>"
                                   "<p>Under GPL3.0 Liscense</p></center>");
    this->ui->textBrowser->setOpenExternalLinks(true);


}

MainWindow::~MainWindow()
{
    this->m_db.close();
    delete ui;
}

/*!
 \brief 获取Cid

 \param cation const 阳离子
 \param cid 传入值，Cid
 \return bool 如果查询正确，则返回true，默认返回false
*/
bool MainWindow::getCid(const QString &cation, QString &cid)
{
    //note: 执行查询，只要执行成功，就返回true，无论返回值是否为空，否则，在qDebug中显示错误信息；
    if(m_query->exec(QString("SELECT Cid FROM Cation WHERE abbreviation = \"%1\"").arg(cation))){
        if(m_query->next()){
            cid = m_query->value(0).toString();
            return true;
        }
    }else
        qDebug() << m_query->lastError();


    this->m_queryList << "Some thing wrong while getting cid<br/>";
    return false;
}

/*!
 \brief 获取Aid

 \param anion const 阴离子
 \param aid 传入值，aid
 \return bool 如果查询正确，则返回true，默认返回false
*/
bool MainWindow::getAid(const QString &anion, QString &aid)
{
    //note: 执行查询，只要执行成功，就返回true，无论返回值是否为空，否则，在qDebug中显示错误信息；
    if(m_query->exec(QString("SELECT Aid FROM Anion WHERE abbreviation =\"%1\"").arg(anion))){
        if(m_query->next()){
            aid = m_query->value(0).toString();
            return true;
        }
    }else
        qDebug() << m_query->lastError();


    this->m_queryList << "Some thing wrong while getting cid<br/>";
    return false;


}

/*!
 \brief 获取基本信息

 \param cid
 \param aid
 \return bool 如果查询成功（无论是否为空），均为true，否则即为false；
*/
bool MainWindow::getBasicInfo(QString &cid, QString &aid)
{
    //note: 防止cid和aid为空
    if(cid.isEmpty() || aid.isEmpty()) return false;

    //note: 本节标题，设置格式为h2
    m_queryList << tr("<h2>Basic Info: </h2>");

    //note: 直接调用SQL语句查询，如果失败，则表明数据库连接异常
    //note: SQL语句："SELECT Abbreviation,Name,Formula,Mol,Ref FROM Basic WHERE Cid = $cid AND Aid = $aid"
    if(this->m_query->exec(QString("SELECT Abbreviation,Name,Formula,Mol,Ref "
                                   "FROM Basic "
                                   "WHERE Cid = \"%1\" "
                                   "AND Aid = \"%2\"  "
                                   ).arg(cid,aid))){

        //note: 在使用QSqlQuery的结果时，默认位置在第一条之前，所以须将指针先移至first，否则所有的结果都为false
        m_query->first();
        if(m_query->isValid()){
            //note: 需要先do，因为每执行一次next()，指针就会后移
            do{
                m_queryList << tr("<p><b>Abbreviation:</b> %1 </p>").arg(m_query->value(0).toString());
                m_queryList << tr("<p><b>Name:</b> %1 </p>").arg(m_query->value(1).toString());
                m_queryList << tr("<p><b>Formula:</b> %1 </p>").arg(m_query->value(2).toString());

                m_queryList << tr("<p><b>Mol:</b> %1 </p>").arg(m_query->value(3).toString());

                //note: Ref需要查询另外的表，故此单独写;表结构(Rid,content)
                m_query_ref->exec(QString("SELECT content FROM Reference WHERE Rid = \"%1\"").arg(m_query->value(4).toString()));
                if(m_query_ref->next()){
                    m_queryList << tr("<p><b>Ref:</b> %1 </p>").arg(m_query_ref->value(0).toString());
                }
            }while(m_query->next());
        }else{
            //note: 即使查询结果为空，也要输出内容
            m_queryList << tr("<h3> No data return </h3>"
                              "<p> Probably because:</p>"
                              "<p> it was not contained in this database</p>"
                              "<p> Or it does not exists.</p>");
        }

        //note: 只要数据库连接正常，就应该返回true；
        return true;
    }


    //note:默认值时false（为了少写一个else）
    qDebug() << m_query->lastError();
    this->m_queryList << "Some thing wrong while getting Basic Info<br/>";

    return false;

}

/*!
 \brief 获取电化学信息，本函数超长，后续可在此函数下添加

 \param cid
 \param aid
 \return bool
*/
bool MainWindow::getECInfo(QString &cid, QString &aid)
{
    //note: 本节标题
    m_queryList << tr("<h2>EC Info: </h2>");
    bool success=false;//note: flag，如果后续中任一查询为真，则为真；
    bool isEmpty=true;//note: flag，如果后续中【所有】查询结果均为空，则返回特定文本；


    /*!
     \note 本段查询Electrochemical_window内容，
            所用SQL语句：
        SELECT "Cathode Limit(V)",
               "Anodic Limit(V)",
               "Electrochemical Window (V)",
               Electrode,
               "Temperature(K)",
               Method,
               Reference
          FROM Electrochemical_window
         WHERE cid = %cid AND
               aid = %aid
    */
    if(m_query->exec(QString("SELECT \"Cathode Limit(V)\",\"Anodic Limit(V)\",\"Electrochemical Window (V)\","
                             "Electrode,\"Temperature(K)\" ,Method,"
                             "Reference "
                             "FROM Electrochemical_window "
                             "WHERE cid = \"%1\" AND aid = \"%2\""
                             ).arg(cid,aid))){


        //note: 此时success的结果应为真；
        success = true;

        //note: 在使用QSqlQuery的结果时，默认位置在第一条之前，所以须将指针先移至first，否则所有的结果都为false
        m_query->first();
        if(m_query->isValid()){
            //note: 此时查询结果不为空，故isEmpty = false;
            isEmpty = false;

            //note: 目标题，格式h3
            m_queryList << tr("<h3>EC Window:</h3>");

            //note: 需要先do，因为每执行一次next()，指针就会后移
            do{
                m_queryList << tr("<p><b>Cathode Limit(V):</b> %1 </p>").arg(m_query->value(0).toString());
                m_queryList << tr("<p><b>Anodic Limit(V):</b> %1 </p>").arg(m_query->value(1).toString());
                m_queryList << tr("<p><b>Electrochemical Window (V):</b> %1 </p>").arg(m_query->value(2).toString());

                m_queryList << tr("<p><b>Electrode:</b> %1 </p>").arg(m_query->value(3).toString());
                m_queryList << tr("<p><b>Temperature(K):</b> %1 </p>").arg(m_query->value(4).toString());

                //note: Method需要查询另外的表，单独写;表结构(Abbreviation,Explanation)
                m_query_ref->exec(QString("SELECT Explanation FROM Method WHERE Abbreviation = \"%1\"").arg(m_query->value(5).toString()));
                if(m_query_ref->next())
                    m_queryList << tr("<p><b>Method:</b> %1 </p>").arg(m_query->value(5).toString());

                //note: Reference需要查询另外的表;表结构(Rid,content)
                m_query_ref->exec(QString("SELECT content FROM Reference WHERE Rid = \"%1\"").arg(m_query->value(6).toString()));
                if(m_query_ref->next())
                    m_queryList << tr("<p><b>Ref:</b> %1 </p>").arg(m_query_ref->value(0).toString());

                //note: 内容过多时的分隔符
                m_queryList<< tr("--------------------------------");
            }while(m_query->next());
        }

    }

    /*!
     \note 本段查询Conductivity内容，
            所用SQL语句：
        SELECT "Conductivity(S/m)",
               "Temperature(℃)",
               "Working Electrode",
               "Reference Electrode",
               "Auxiliary Electrode",
               Method,
               Reference
          FROM Conductivity
         WHERE cid = %cid AND
               aid = %aid
    */
    if(m_query->exec(QString("SELECT \"Conductivity(S/m)\",\"Temperature(℃)\",\"Working Electrode\","
                             "\"Reference Electrode\",\"Auxiliary Electrode\",Method,"
                             "Reference "
                             "FROM Conductivity "
                             "WHERE cid = \"%1\" AND aid = \"%2\""
                             ).arg(cid,aid))){

        //note: 此时success的结果应为真；
        success = true||success;

        //note: 在使用QSqlQuery的结果时，默认位置在第一条之前，所以须将指针先移至first，否则所有的结果都为false
        m_query->first();
        if(m_query->isValid()){
            //note: 此时查询结果不为空，故isEmpty = false;
            isEmpty = false;

            //note: 目标题，格式h3
            m_queryList << tr("<h3>Conductivity:</h3>");

            //note: 需要先do，因为每执行一次next()，指针就会后移
            do{
                m_queryList << tr("<p><b>Conductivity(S/m):</b> %1 </p>").arg(m_query->value(0).toString());
                m_queryList << tr("<p><b>Temperature(℃):</b> %1 </p>").arg(m_query->value(1).toString());
                m_queryList << tr("<p><b>Working Electrode:</b> %1 </p>").arg(m_query->value(2).toString());

                m_queryList << tr("<p><b>Reference Electrode:</b> %1 </p>").arg(m_query->value(3).toString());
                m_queryList << tr("<p><b>Auxiliary Electrode:</b> %1 </p>").arg(m_query->value(4).toString());

                //note: Method需要查询另外的表，单独写;表结构(Abbreviation,Explanation)
                m_query_ref->exec(QString("SELECT Explanation FROM Method WHERE Abbreviation = \"%1\"").arg(m_query->value(5).toString()));
                if(m_query_ref->next())
                    m_queryList << tr("<p><b>Method:</b> %1 </p>").arg(m_query->value(5).toString());

                //note: Reference需要查询另外的表，单独写;表结构(Rid,content)
                m_query_ref->exec(QString("SELECT content FROM Reference WHERE Rid = \"%1\"").arg(m_query->value(6).toString()));
                if(m_query_ref->next())
                    m_queryList << tr("<p><b>Ref:</b> %1 </p>").arg(m_query_ref->value(0).toString());

                //note: 内容过多时的分隔符
                m_queryList<< tr("--------------------------------");
            }while(m_query->next());

        }


    }

    /*!
     \note 本段查询Conductivity_Ils_water内容，
            所用SQL语句：
        SELECT "Specific Conductivity",
               "W(water)",
               "T(K)",
          FROM Conductivity_Ils_water
         WHERE cid = %cid AND
               aid = %aid
    */
    if(m_query->exec(QString("SELECT \"Specific Conductivity\",\"W(water)\",\"T(K)\","
                             "Reference "
                             "FROM Conductivity_Ils_water "
                             "WHERE cid = \"%1\" AND aid = \"%2\""
                             ).arg(cid,aid))){

        //note: 此时success的结果应为真；
        success = true ||success;

        //note: 在使用QSqlQuery的结果时，默认位置在第一条之前，所以须将指针先移至first，否则所有的结果都为false
        m_query->first();
        if(m_query->isValid()){
            //note: 此时查询结果不为空，故isEmpty = false;
            isEmpty = false;

            //note: 目标题，格式h3
            m_queryList << tr("<h3>Conductivity_Ils_water:</h3>");

            //note: 需要先do，因为每执行一次next()，指针就会后移
            do{
                m_queryList << tr("<p><b>Specific Conductivity:</b> %1 </p>").arg(m_query->value(0).toString());
                m_queryList << tr("<p><b>W(water):</b> %1 </p>").arg(m_query->value(1).toString());
                m_queryList << tr("<p><b>T(K):</b> %1 </p>").arg(m_query->value(2).toString());

                //note: Method需要查询另外的表，单独写;表结构(Rid,content)
                m_query_ref->exec(QString("SELECT content FROM Reference WHERE Rid = \"%1\"").arg(m_query->value(3).toString()));
                if(m_query_ref->next())
                    m_queryList << tr("<p><b>Ref:</b> %1 </p>").arg(m_query_ref->value(0).toString());

                //note: 内容过多时的分隔符
                m_queryList<< tr("--------------------------------");
            }while(m_query->next());

        }
    }

    /*!
     \note 本段查询Diffusion Coefficientr内容，
            所用SQL语句：
        SELECT "Diffusion Coefficient（10-10 m2/s）",
               "Temperature(K)",
               "Reference",
          FROM Diffusion_coefficient
         WHERE cid = %cid AND
               aid = %aid
    */
    if(m_query->exec(QString("SELECT \"Diffusion Coefficient（10-10 m2/s）\",\"Temperature(K)\","
                             "Reference "
                             "FROM Diffusion_coefficient "
                             "WHERE cid = \"%1\" AND aid = \"%2\""
                             ).arg(cid,aid))){

        //note: 此时success的结果应为真；
        success = true ||success;

        //note: 在使用QSqlQuery的结果时，默认位置在第一条之前，所以须将指针先移至first，否则所有的结果都为false
        m_query->first();
        if(m_query->isValid()){
            //note: 此时查询结果不为空，故isEmpty = false;
            isEmpty = false;

            //note: 目标题，格式h3
            m_queryList << tr("<h3>Diffusion_coefficient:</h3>");

            //note: 需要先do，因为每执行一次next()，指针就会后移
            do{
                m_queryList << tr("<p><b>Diffusion Coefficient（10<sup>-10</sup> m<sup>2</sup>/s）:</b> %1 </p>").arg(m_query->value(0).toString());
                m_queryList << tr("<p><b>Temperature(K):</b> %1 </p>").arg(m_query->value(1).toString());

                //note: Reference需要查询另外的表，单独写;表结构(Rid,content)
                m_query_ref->exec(QString("SELECT content FROM Reference WHERE Rid = \"%1\"").arg(m_query->value(2).toString()));
                if(m_query_ref->next())
                    m_queryList << tr("<p><b>Ref:</b> %1 </p>").arg(m_query_ref->value(0).toString());

                //note: 内容过多时的分隔符
                m_queryList<< tr("--------------------------------");
            }while(m_query->next());

        }
    }

    /*!
     \note 本段查询Kinematic_viscosity内容，
            所用SQL语句：
        SELECT "Kinematic Viscosity（mm2/S)",
               "Method",
               "Reference",
          FROM Kinematic_viscosity
         WHERE cid = %cid AND
               aid = %aid
    */
    if(m_query->exec(QString("SELECT \"Kinematic Viscosity（mm<sup>2</sup>/S)\",\"Method\",Reference "
                             "FROM Kinematic_viscosity "
                             "WHERE cid = \"%1\" AND aid = \"%2\""
                             ).arg(cid,aid))){

        //note: 此时success的结果应为真；
        success = true ||success;

        //note: 在使用QSqlQuery的结果时，默认位置在第一条之前，所以须将指针先移至first，否则所有的结果都为false
        m_query->first();
        if(m_query->isValid()){
            //note: 此时查询结果不为空，故isEmpty = false;
            isEmpty = false;

            //note: 目标题，格式h3
            m_queryList << tr("<h3>Kinematic_viscosity:</h3>");

            //note: 需要先do，因为每执行一次next()，指针就会后移
            do{
                m_queryList << tr("<p><b>Kinematic Viscosity（mm《sup>2</sup>/S):</b> %1 </p>").arg(m_query->value(0).toString());

                //note: Method需要查询另外的表，单独写;表结构(Abbreviation,Explanation)
                m_query_ref->exec(QString("SELECT Explanation FROM Method WHERE Abbreviation = \"%1\"").arg(m_query->value(1).toString()));
                if(m_query_ref->next())
                    m_queryList << tr("<p><b>Method:</b> %1 </p>").arg(m_query->value(5).toString());

                //note: Method需要查询另外的表，单独写;表结构(Rid,content)
                m_query_ref->exec(QString("SELECT content FROM Reference WHERE Rid = \"%1\"").arg(m_query->value(2).toString()));
                if(m_query_ref->next())
                    m_queryList << tr("<p><b>Ref:</b> %1 </p>").arg(m_query_ref->value(0).toString());

                //note: 内容过多时的分隔符
                m_queryList<< tr("--------------------------------");
            }while(m_query->next());

        }
    }


    //note: 如果内容为空，输出可能原因
    if(isEmpty)
        m_queryList << tr("<h3> No data return </h3>"
                          "<p> Probably because:</p>"
                          "<p> it was not contained in this database</p>"
                          "<p> Or corresponding data does not exists.</p>");

    //note: !success 一般由于数据库连接问题
    if(!success){
        qDebug() << m_query->lastError();
        this->m_queryList << "Some thing wrong while getting EC Info<br/>";
    }

    return success;
}

/*!
 \brief 为阳离子创建completer
 \note 首先清空m_queryList，接着查询cation表，获取所有的cation；创建completer，初始化并设置相应属性。
*/
void MainWindow::setCompleter()
{

    //阳离子
    this->m_queryList.clear();
    if(!m_query->exec("SELECT Abbreviation FROM Cation")){
        qDebug() << "Error: cannot select elements from Cation" << m_query->lastError();
    }else{
        while(m_query->next())
            this->m_queryList << m_query->value(0).toString();
    }
    QCompleter *cmplter = new QCompleter(m_queryList);
    cmplter->setCaseSensitivity(Qt::CaseInsensitive);
    cmplter->setFilterMode(Qt::MatchContains);
    ui->leCation->setCompleter(cmplter);

    //阴离子
    this->m_queryList.clear();
    if(!m_query->exec("SELECT Abbreviation FROM Anion")){
        qDebug() << "Error: cannot select elements from Basic" << m_query->lastError();
    }else{
        while(m_query->next()){
            this->m_queryList << m_query->value(0).toString();
        }
        //        qDebug() << this->m_queryList;
    }
    cmplter = new QCompleter(m_queryList);
    cmplter->setCaseSensitivity(Qt::CaseInsensitive);
    cmplter->setFilterMode(Qt::MatchContains);
    ui->leAnion->setCompleter(cmplter);

}

bool MainWindow::getThermoInfo(QString &cid, QString &aid)
{
    //TODO:


    if(true){

        return true;
    }
    m_queryList << "Some thing wrong while getting Thermo Info<br/>";
    return false;

}

void MainWindow::on_pushButton_clicked()
{


    if(ui->leAnion->text().isEmpty() || ui->leCation->text().isEmpty()) return;

    QString cid="";
    QString aid="";

    this->m_queryList.clear();

    //note:如果成功获取cid和aid，则可进行下一步，否则什么也不返回
    if(getCid(ui->leCation->text(),cid) && getAid(ui->leAnion->text(),aid)){
        getBasicInfo(cid,aid);
        if(this->ui->ckBThermo->isChecked())
            getThermoInfo(cid,aid);
        if(this->ui->ckBEC->isChecked())
            getECInfo(cid,aid);

        //note: 把cid和aid输出，方便调试
        qDebug() << cid << aid;
        //note:结果呈现
//        ui->textBrowser->setAlignment(Qt::AlignLeft|Qt::AlignTop);
        ui->textBrowser->setHtml(m_queryList.join("").toStdString().c_str());

    }
}
