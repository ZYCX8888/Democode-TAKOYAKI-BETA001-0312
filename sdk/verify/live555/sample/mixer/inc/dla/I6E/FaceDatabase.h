#pragma once
#ifndef _FACE_DATABASE_H
#define _FACE_DATABASE_H
#include "FaceFeatureh.h"
#include <vector>
#include <math.h>

template<class T>
static T Length2(int n, const T* data)
{
    T result = 0;
    for (int i = 0; i < n; i++)
        result += data[i] * data[i];
    return result;
}

template<class T>
static T Normalize(int n, T* data)
{
    T len2 = Length2(n, data);
    if (len2 == 0)
        return 0;

    T len = sqrt((double)len2);
    for (int i = 0; i < n; i++)
        data[i] /= len;
    return len;
}



class FaceDatabase
{
public:
    class FaceFeature
    {
    public:
        int length;
        float pData[128];
    };

    class Person
    {
    public:
        std::vector<FaceFeature> features;
        std::string name;
    };

    public:
        std::vector<Person> persons;

        void Clear()
        {
            persons.clear();
        }
        void AddPersonFeature(const char* name, float featureData[128])
        {
           unsigned int i;
           FaceFeature feature;
           feature.length=128;
           memcpy(feature.pData,featureData,sizeof(float)*feature.length);
           Normalize<float>(128, feature.pData);
           //printf("save feature:%f_%f_%f_%f\n",feature.pData[0],feature.pData[1],feature.pData[2],feature.pData[3]);
           for(i = 0; i<persons.size();i++)
           {
             if(strcmp(persons[i].name.c_str(),name)==0)
             {
                 persons[i].features.push_back(feature);
                 //SaveToFileBinary();
                 return;
              }
            }

           Person person_temp;
           person_temp.name=name;
           persons.push_back(person_temp);
           persons[persons.size()-1].features.push_back(feature);
           #if 0
           printf("save name:%s\n",persons[persons.size()-1].name.c_str());
           int feature_index = persons[persons.size()-1].features.size()-1;
           printf("feature_index:%d\n",feature_index);
           if (feature_index!=-1)
           {
                FaceFeature feature_temp =  persons[persons.size()-1].features[feature_index];
                printf("feature_data:%f_%f_%f_%f\n",feature_temp.pData[0],feature_temp.pData[1],feature_temp.pData[2],feature_temp.pData[3]);
           }
           #endif
        }
        void DelPerson(const char *name)
        {
              std::vector<Person>::iterator it;
              for(it = persons.begin(); it<persons.end();)
              {
                 if(strcmp(it->name.c_str(),name)==0)
                 {
                     persons.erase(it);
                       // SaveToFileBinary();
                     return;
                 }
                 else
                 {
                    it++;
                 }
              }

        }

        bool SaveToFileBinary(const std::string& feats_file, const std::string& names_file)
        {
            if (!_check_valid())
            {
                printf("not a valid database\n");
                return false;
            }
            if (!_write_feats_binary(feats_file))
            {
                printf("failed to save %s\n", feats_file.c_str());
                return false;
            }
            if (!_write_names(names_file))
            {
                printf("failed to save %s\n", names_file.c_str());
                return false;
            }

            printf("save database to bin \n");
            return true;
        }

        bool LoadFromFileBinay(const std::string& feats_file, const std::string& names_file)
        {
            Clear();
            if (!_load_feats_binary(feats_file))
            {
                Clear();
                return false;
            }

            if (!_load_names(names_file))
            {
                Clear();
                return false;
            }
            return true;
        }


    private:
        bool _check_valid()
        {
            int person_num = persons.size();
            if (person_num == 0)
                return false;

            for (int i = 0; i < person_num; i++)
            {
                int feat_num = persons[i].features.size();
                if (feat_num == 0)
                    return false;
            }
            int feat_dim = persons[0].features[0].length;
            if (feat_dim == 0)
                return false;
            for (int i = 0; i < person_num; i++)
            {
                int feat_num = persons[i].features.size();
                for (int j = 0; j < feat_num; j++)
                {
                    if (feat_dim != persons[i].features[j].length)
                        return false;
                }
            }
            return true;
        }

        bool _write_feats_binary(const std::string& file)
        {
            FILE* out = 0;
            out = fopen( file.c_str(), "wb");

            int person_num = persons.size();
            unsigned int feat_dim = persons[0].features[0].length;

            if (1 != fwrite(&feat_dim, sizeof(int), 1, out))
            {
                fclose(out);
                return false;
            }
            if (1 != fwrite(&person_num, sizeof(int), 1, out))
            {
                fclose(out);
                return false;
            }

            for (int i = 0; i < person_num; i++)
            {
                int feat_num = persons[i].features.size();
                if (1 != fwrite(&feat_num, sizeof(int), 1, out))
                {
                    fclose(out);
                    return false;
                }

                for (int j = 0; j < feat_num; j++)
                {
                    if (feat_dim != fwrite(persons[i].features[j].pData, sizeof(float), feat_dim, out))
                    {
                        fclose(out);
                        return false;
                    }
                }
            }
            fclose(out);
            return true;
        }

        bool _write_names(const std::string& file)
        {
            FILE* out = 0;


            out = fopen(file.c_str(), "w");
            int person_num = persons.size();
            for (int i = 0; i < person_num; i++)
            {
                fprintf(out, "%s\n", persons[i].name.c_str());
            }
            fclose(out);
            return true;
        }

        bool _load_feats_binary(const std::string& file)
        {
            FILE* in = 0;
            in = fopen(file.c_str(), "rb");
            int person_num = 0;
            unsigned int feat_dim = 0;

            if (1 != fread(&feat_dim, sizeof(int), 1, in))
            {
                fclose(in);
                return false;
            }
            if (1 != fread(&person_num, sizeof(int), 1, in))
            {
                fclose(in);
                return false;
            }
            if (person_num <= 0 || feat_dim <= 0)
            {
                fclose(in);
                return false;
            }
            persons.resize(person_num);
            int start = 0;
            if (person_num > 20) { start = person_num - 20; }

            for (int i = start; i < person_num; i++)
            {
                int feat_num = 0;

                if (1 != fread(&feat_num, sizeof(int), 1, in))
                {
                    fclose(in);
                    return false;
                }
                if (feat_num <= 0)
                {
                    fclose(in);
                    return false;
                }
                persons[i].features.resize(feat_num);
                for (int j = 0; j < feat_num; j++)
                {
                    if (feat_dim != fread(persons[i].features[j].pData, sizeof(float), feat_dim, in))
                    {
                        fclose(in);
                        return false;
                    }
                    persons[i].features[j].length = feat_dim;
                }
            }
            fclose(in);
            return true;
        }

        bool _load_names(const std::string& file)
        {
            FILE* in = 0;

            in = fopen( file.c_str(), "r");
            char line[200] = { 0 };
            int i = 0;
            while (true)
            {
                line[0] = '\0';
                fgets(line, 199, in);
                if (line[0] == '\0')
                    break;
                int len = strlen(line);
                if (line[len - 1] == '\n')
                    line[--len] = '\0';
                persons[i].name = line;
                i++;
            }
            fclose(in);
            return true;
        }

};

#endif