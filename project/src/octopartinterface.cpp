#include <QJsonDocument>
#include <QDebug>
#include <QByteArray>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QFile>
#include "octopartinterface.h"

OctopartInterface::OctopartInterface(QString apikey, QObject *parent) : QObject(parent), restRequester(parent)
{
    this->apikey = apikey;
    connect(&restRequester, SIGNAL(http_request_finished()),
            this, SLOT(http_request_finished()));

}

OctopartInterface::~OctopartInterface()
{

}

void OctopartInterface::sendMPNQuery(OctopartCategorieCache &octopartCategorieCache, QString mpn)
{

    QString url_str = "https://octopart.com/api/v3/parts/match";

    QMultiMap<QString,QString> map;

    map.insert("apikey", apikey);
    map.insert("queries", "[{\"mpn\":\""+mpn+"\"}]");

    map.insert("pretty_print", "true");
    map.insert("include[]", "category_uids");
    map.insert("include[]", "short_description");
    map.insert("include[]", "specs");
    map.insert("include[]","cad_models");


    //map.insert("include[]","external_links");
    map.insert("include[]", "datasheets");

    // map.insert("hide[]", "offers");
    // map.insert("show[]", "offers");

    octopartResult_QueryMPN.clear();


    QBuffer buffer;
    restRequester.startRequest(url_str,map, &buffer);
    buffer.open(QIODevice::ReadOnly);
    QByteArray resultData = buffer.readAll();
    QJsonDocument loadDoc = QJsonDocument::fromJson(resultData);

    QJsonObject jsonObject(loadDoc.object());


    octopartResult_QueryMPN.clear();
    QJsonArray jsonArray = jsonObject["results"].toArray();
    QJsonObject jsonItem = jsonArray[0].toObject();
    QJsonArray jsonItemsArray = jsonItem["items"].toArray();

    foreach (const QJsonValue & value, jsonItemsArray) {
        QJsonObject obj = value.toObject();
        OctopartResult_QueryMPN_Entry entry;
        entry.mpn = obj["mpn"].toString();
        entry.manufacturer = obj["manufacturer"].toObject()["name"].toString();
        entry.footprint = obj["specs"].toObject()["case_package"].toObject()["display_value"].toString();
        entry.description = obj["short_description"].toString();
        QJsonArray categorie_Array = obj["category_uids"].toArray();

        foreach (const QJsonValue & categorie, categorie_Array) {
            QString categorie_str = categorie.toString();
            OctopartCategorie octopartCategorie = getCategorie(octopartCategorieCache, categorie_str);
           // qDebug() << octopartCategorie.categorieNameTree;
            entry.categories.append(octopartCategorie);
        }

#if 0
"specs": {
    "resistance": {
        "__class__": "SpecValue",
        "attribution": {
            "__class__": "Attribution",
            "first_acquired": null,
            "sources": null
        },
        "display_value": "22.0 k\u03a9",
        "max_value": null,
        "metadata": {
            "__class__": "SpecMetadata",
            "datatype": "decimal",
            "key": "resistance",
            "name": "Resistance",
            "unit": {
                "__class__": "UnitOfMeasurement",
                "name": "ohms",
                "symbol": "\u03a9"
            }
        },
        "min_value": null,
        "value": [
            "22000.0"
        ]
    },
    #endif

        QJsonArray cad_object=    obj["cad_models"].toArray();
        entry.url3DModel = cad_object[0].toObject()["url"].toString();
       // qDebug() << "3dmodel" << entry.url3DModel;
        QJsonObject specs_object = obj["specs"].toObject();

        foreach (const QJsonValue & spec_entry, specs_object) {
            QJsonObject specObject = spec_entry.toObject();
            QJsonObject metaObject = specObject["metadata"].toObject();
            QString key = metaObject["key"].toString();

            OctopartSpecEntry specEntry;
            specEntry.name = metaObject["name"].toString();
            specEntry.dataType = metaObject["datatype"].toString();

            QJsonObject unitObject = metaObject["unit"].toObject();
            specEntry.unitName = unitObject["name"].toString();
            specEntry.unitSymbol = unitObject["symbol"].toString();

            specEntry.value = specObject["value"].toArray()[0].toVariant();
            specEntry.min_value = specObject["min_value"].toVariant();
            specEntry.max_value = specObject["max_value"].toVariant();
            specEntry.displayValue = specObject["display_value"].toString();

           // qDebug() << specEntry.toString();
            entry.specs.insert(key,specEntry);
        }
        entry.urlDataSheet = obj["datasheets"].toArray()[0].toObject()["url"].toString();
        entry.urlOctoPart = obj["octopart_url"].toString();

        octopartResult_QueryMPN.append(entry);
#if 0
        qDebug() << "mpn" << entry.mpn;
        qDebug() << "manufacturer" << entry.manufacturer;
        qDebug() << "footprint" << entry.footprint;
        qDebug() << "description" << entry.description;
        qDebug() << "urlDataSheet" << entry.urlDataSheet;
        qDebug() << "urlOctoPart" << entry.urlOctoPart;
#endif


        // qDebug() << octopartResult_QueryMPN;
    }

    QFile jsonfile("jsonfile.txt");
    jsonfile.open(QIODevice::WriteOnly);
    jsonfile.write(resultData);
    jsonfile.close();

}

OctopartCategorie OctopartInterface::getCategorie(OctopartCategorieCache &cache, QString category_id){
    OctopartCategorie result = cache.fetch(category_id);
    if (result.isEmpty()){
        result = getCategorieByRequest(category_id);
        cache.addCategorie(result);
    }
    return result;
}

OctopartCategorie OctopartInterface::getCategorieByRequest(QString category_id)
{
    QString url_str = "https://octopart.com/api/v3/categories/"+category_id;
    OctopartCategorie result;
    QMap<QString,QString> map;

    map.insert("apikey", apikey);

    map.insert("pretty_print", "true");



    QBuffer buffer;
    restRequester.startRequest(url_str,map, &buffer);
    buffer.open(QIODevice::ReadOnly);

    QByteArray resultData = buffer.readAll();
    QJsonDocument loadDoc = QJsonDocument::fromJson(resultData);
    QJsonObject jsonObject(loadDoc.object());


    QJsonArray jsonAncestorNames = jsonObject["ancestor_names"].toArray();
    // qDebug() << jsonAncestorNames;
    foreach (const QJsonValue & value, jsonAncestorNames) {
        result.categorieNameTree.append(value.toString());
    }
    result.categorie_uid = jsonObject["uid"].toString();
    result.categorieNameTree.append(jsonObject["name"].toString());
    qDebug() << result.categorieNameTree;

    QFile jsonfile("jsonfile.txt");
    jsonfile.open(QIODevice::WriteOnly);
    jsonfile.write(resultData);
    jsonfile.close();
    return result;

    /*{
    "__class__": "Category",
    "ancestor_names": [
        "Electronic Parts",
        "Passive Components",
        "Resistors"
    ],
    "ancestor_uids": [
        "8a1e4714bb3951d9",
        "7542b8484461ae85",
        "5c6a91606d4187ad"
    ],
    "children_uids": [],
    "name": "Chip SMD Resistors",
    "num_parts": 653581,
    "parent_uid": "5c6a91606d4187ad",
    "uid": "a2f46ffe9b377933"
}*/
}


void OctopartInterface::setAPIKey(QString apikey)
{
    this->apikey = apikey;
}

void OctopartInterface::http_request_finished()
{


}


void OctopartCategorie::clear()
{
    categorie_uid = "";
    categorieNameTree.clear();
}

bool OctopartCategorie::isEmpty()
{
    return categorieNameTree.length() == 0 || categorie_uid == "";
}

OctopartCategorieCache::OctopartCategorieCache() : cache("octopartCategorieCache.ini",
                                                         QSettings::IniFormat)
{

}

void OctopartCategorieCache::save()
{
    cache.sync();
}



OctopartCategorie OctopartCategorieCache::fetch(QString categorie_uid)
{
    OctopartCategorie result;
    QStringList groups = cache.childGroups();

    if (groups.contains(categorie_uid)){

       cache.beginGroup(categorie_uid);
       result.categorie_uid = categorie_uid;
       result.categorieNameTree = cache.value("categorieNameTree").toStringList();

       cache.endGroup();
    }
    return result;
}

void OctopartCategorieCache::addCategorie(OctopartCategorie octopartCategorie)
{
    cache.beginGroup(octopartCategorie.categorie_uid);
    cache.setValue("categorieNameTree",octopartCategorie.categorieNameTree);


    cache.endGroup();
}





OctopartSpecEntry::OctopartSpecEntry()
{

}

QString OctopartSpecEntry::toString()
{


    return  "name:"+name+", unitName:"+unitName+", unitSymbol:"+unitSymbol+", dataType:"+dataType+
            ", value:"+value.toString()+", min_value:"+min_value.toString()+", max_value:"+max_value.toString()+", displayValue:"+displayValue;
}

QString unitPrefixFromExponent(int exponent){
    switch(exponent){
    case 24: return "Y";
        break;
    case 21: return "Z";
        break;
    case 18: return "E";
        break;
    case 15: return "P";
        break;
    case 12: return "T";
        break;
    case 9: return "G";
        break;
    case 6: return "M";
        break;
    case 3: return "k";
        break;
    case -3: return "m";
        break;
    case -6: return "u";
        break;
    case -9: return "n";
        break;
    case -12: return "p";
        break;
    }
    return "";
}

QString getNiceUnitPrefix(double val,QString &prefix){
    int e=0;
    if (val >= 1000){
        while (val >= 1000){
            val /=1000;
            e+=3;
        }
    }else if (val == 0){
    }else if (val < 1){
        while (val < 1){
            val *=1000;
            e-=3;
        }
    }

    prefix = unitPrefixFromExponent(e);
    QString result = QString::number(val);
    return result;
}

void OctopartResult_QueryMPN_Entry::clear()
{
    mpn.clear();
    manufacturer.clear();
    description.clear();
    footprint.clear();

    urlOctoPart.clear();
    urlDataSheet.clear();
    url3DModel.clear();
    categories.clear();

    specs.clear();

}

QMap<QString, QString> OctopartResult_QueryMPN_Entry::getQueryResultMap()
{
    QMap<QString, QString> result;


    result.insert("%octo.mpn%",mpn);
    result.insert("%octo.manufacturer%",manufacturer);
    result.insert("%octo.description%",description);
    result.insert("%octo.footprint%",footprint);



    QMapIterator<QString, OctopartSpecEntry> i(specs);
    while (i.hasNext()) {
        i.next();
        result.insert("%octo.spec."+i.key()+".name%",i.value().name);
        result.insert("%octo.spec."+i.key()+".value%",i.value().value.toString().remove("±"));
        result.insert("%octo.spec."+i.key()+".dispval%",i.value().displayValue);
        result.insert("%octo.spec."+i.key()+".unitname%",i.value().unitName);
        result.insert("%octo.spec."+i.key()+".unitsymbol%",i.value().unitSymbol);
        result.insert("%octo.spec."+i.key()+".minvalue%",i.value().min_value.toString());
        result.insert("%octo.spec."+i.key()+".maxvalue%",i.value().max_value.toString());


        QString prefix;
        QString niceVal = getNiceUnitPrefix(i.value().value.toDouble(),prefix);
        if (!niceVal.contains(".")){
            niceVal += ".";
        }
        niceVal = niceVal.replace(".",prefix);
        result.insert("%octo.spec."+i.key()+".nicenum%",  niceVal);
    }



    return result;
}