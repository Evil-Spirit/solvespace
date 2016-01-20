//-----------------------------------------------------------------------------
// Routines to write and read our .slvs file format.
//
// Copyright 2008-2013 Jonathan Westhues.
//-----------------------------------------------------------------------------
#include "solvespace.h"

#define VERSION_STRING "\261\262\263" "SolveSpaceREVa"

static int StrStartsWith(const char *str, const char *start) {
    return memcmp(str, start, strlen(start)) == 0;
}

//-----------------------------------------------------------------------------
// Clear and free all the dynamic memory associated with our currently-loaded
// sketch. This does not leave the program in an acceptable state (with the
// references created, and so on), so anyone calling this must fix that later.
//-----------------------------------------------------------------------------
void SolveSpaceUI::ClearExisting(void) {
    UndoClearStack(&redo);
    UndoClearStack(&undo);

    Group *g;
    for(g = sketch->group.First(); g; g = sketch->group.NextAfter(g)) {
        g->Clear();
    }

    sketch->constraint.Clear();
    sketch->request.Clear();
    sketch->group.Clear();
    sketch->style.Clear();

    sketch->entity.Clear();
    sketch->param.Clear();
}

hGroup SolveSpaceUI::CreateDefaultDrawingGroup(void) {
    Group g { sketch };

    // And an empty group, for the first stuff the user draws.
    g.visible = true;
    g.type = Group::DRAWING_WORKPLANE;
    g.subtype = Group::WORKPLANE_BY_POINT_ORTHO;
    g.predef.q = Quaternion::From(1, 0, 0, 0);
    hRequest hr = Request::HREQUEST_REFERENCE_XY;
    g.predef.origin = hr.entity(1);
    g.name = "sketch-in-plane";
    sketch->group.AddAndAssignId(&g);
    sketch->GetGroup(g.h)->activeWorkplane = g.h.entity(0);
    return g.h;
}

void SolveSpaceUI::NewFile(void) {
    ClearExisting();

    // Our initial group, that contains the references.
    Group g { sketch };
    g.visible = true;
    g.name = "#references";
    g.type = Group::DRAWING_3D;
    g.h = Group::HGROUP_REFERENCES;
    sketch->group.Add(&g);

    // Let's create three two-d coordinate systems, for the coordinate
    // planes; these are our references, present in every sketch.
    Request r { sketch };
    r.type = Request::WORKPLANE;
    r.group = Group::HGROUP_REFERENCES;
    r.workplane = Entity::FREE_IN_3D;

    r.h = Request::HREQUEST_REFERENCE_XY;
    sketch->request.Add(&r);

    r.h = Request::HREQUEST_REFERENCE_YZ;
    sketch->request.Add(&r);

    r.h = Request::HREQUEST_REFERENCE_ZX;
    sketch->request.Add(&r);

    CreateDefaultDrawingGroup();
}

const SolveSpaceUI::SaveTable SolveSpaceUI::SAVED[] {
    { 'g',  "Group.h.v",                'x',    &(SS.sv.g.h.v)                },
    { 'g',  "Group.type",               'd',    &(SS.sv.g.type)               },
    { 'g',  "Group.order",              'd',    &(SS.sv.g.order)              },
    { 'g',  "Group.name",               'S',    &(SS.sv.g.name)               },
    { 'g',  "Group.activeWorkplane.v",  'x',    &(SS.sv.g.activeWorkplane.v)  },
    { 'g',  "Group.opA.v",              'x',    &(SS.sv.g.opA.v)              },
    { 'g',  "Group.opB.v",              'x',    &(SS.sv.g.opB.v)              },
    { 'g',  "Group.valA",               'f',    &(SS.sv.g.valA)               },
    { 'g',  "Group.valB",               'f',    &(SS.sv.g.valB)               },
    { 'g',  "Group.valC",               'f',    &(SS.sv.g.valB)               },
    { 'g',  "Group.color",              'c',    &(SS.sv.g.color)              },
    { 'g',  "Group.subtype",            'd',    &(SS.sv.g.subtype)            },
    { 'g',  "Group.skipFirst",          'b',    &(SS.sv.g.skipFirst)          },
    { 'g',  "Group.meshCombine",        'd',    &(SS.sv.g.meshCombine)        },
    { 'g',  "Group.forceToMesh",        'd',    &(SS.sv.g.forceToMesh)        },
    { 'g',  "Group.predef.q.w",         'f',    &(SS.sv.g.predef.q.w)         },
    { 'g',  "Group.predef.q.vx",        'f',    &(SS.sv.g.predef.q.vx)        },
    { 'g',  "Group.predef.q.vy",        'f',    &(SS.sv.g.predef.q.vy)        },
    { 'g',  "Group.predef.q.vz",        'f',    &(SS.sv.g.predef.q.vz)        },
    { 'g',  "Group.predef.origin.v",    'x',    &(SS.sv.g.predef.origin.v)    },
    { 'g',  "Group.predef.entityB.v",   'x',    &(SS.sv.g.predef.entityB.v)   },
    { 'g',  "Group.predef.entityC.v",   'x',    &(SS.sv.g.predef.entityC.v)   },
    { 'g',  "Group.predef.swapUV",      'b',    &(SS.sv.g.predef.swapUV)      },
    { 'g',  "Group.predef.negateU",     'b',    &(SS.sv.g.predef.negateU)     },
    { 'g',  "Group.predef.negateV",     'b',    &(SS.sv.g.predef.negateV)     },
    { 'g',  "Group.visible",            'b',    &(SS.sv.g.visible)            },
    { 'g',  "Group.suppress",           'b',    &(SS.sv.g.suppress)           },
    { 'g',  "Group.relaxConstraints",   'b',    &(SS.sv.g.relaxConstraints)   },
    { 'g',  "Group.allDimsReference",   'b',    &(SS.sv.g.allDimsReference)   },
    { 'g',  "Group.scale",              'f',    &(SS.sv.g.scale)              },
    { 'g',  "Group.remap",              'M',    &(SS.sv.g.remap)              },
    { 'g',  "Group.impFile",            'S',    &(SS.sv.g.impFile)            },
    { 'g',  "Group.impFileRel",         'S',    &(SS.sv.g.impFileRel)         },

    { 'p',  "Param.h.v.",               'x',    &(SS.sv.p.h.v)                },
    { 'p',  "Param.val",                'f',    &(SS.sv.p.val)                },

    { 'r',  "Request.h.v",              'x',    &(SS.sv.r.h.v)                },
    { 'r',  "Request.type",             'd',    &(SS.sv.r.type)               },
    { 'r',  "Request.extraPoints",      'd',    &(SS.sv.r.extraPoints)        },
    { 'r',  "Request.workplane.v",      'x',    &(SS.sv.r.workplane.v)        },
    { 'r',  "Request.group.v",          'x',    &(SS.sv.r.group.v)            },
    { 'r',  "Request.construction",     'b',    &(SS.sv.r.construction)       },
    { 'r',  "Request.style",            'x',    &(SS.sv.r.style)              },
    { 'r',  "Request.str",              'S',    &(SS.sv.r.str)                },
    { 'r',  "Request.font",             'S',    &(SS.sv.r.font)               },

    { 'e',  "Entity.h.v",               'x',    &(SS.sv.e.h.v)                },
    { 'e',  "Entity.type",              'd',    &(SS.sv.e.type)               },
    { 'e',  "Entity.construction",      'b',    &(SS.sv.e.construction)       },
    { 'e',  "Entity.style",             'x',    &(SS.sv.e.style)              },
    { 'e',  "Entity.str",               'S',    &(SS.sv.e.str)                },
    { 'e',  "Entity.font",              'S',    &(SS.sv.e.font)               },
    { 'e',  "Entity.point[0].v",        'x',    &(SS.sv.e.point[0].v)         },
    { 'e',  "Entity.point[1].v",        'x',    &(SS.sv.e.point[1].v)         },
    { 'e',  "Entity.point[2].v",        'x',    &(SS.sv.e.point[2].v)         },
    { 'e',  "Entity.point[3].v",        'x',    &(SS.sv.e.point[3].v)         },
    { 'e',  "Entity.point[4].v",        'x',    &(SS.sv.e.point[4].v)         },
    { 'e',  "Entity.point[5].v",        'x',    &(SS.sv.e.point[5].v)         },
    { 'e',  "Entity.point[6].v",        'x',    &(SS.sv.e.point[6].v)         },
    { 'e',  "Entity.point[7].v",        'x',    &(SS.sv.e.point[7].v)         },
    { 'e',  "Entity.point[8].v",        'x',    &(SS.sv.e.point[8].v)         },
    { 'e',  "Entity.point[9].v",        'x',    &(SS.sv.e.point[9].v)         },
    { 'e',  "Entity.point[10].v",       'x',    &(SS.sv.e.point[10].v)        },
    { 'e',  "Entity.point[11].v",       'x',    &(SS.sv.e.point[11].v)        },
    { 'e',  "Entity.extraPoints",       'd',    &(SS.sv.e.extraPoints)        },
    { 'e',  "Entity.normal.v",          'x',    &(SS.sv.e.normal.v)           },
    { 'e',  "Entity.distance.v",        'x',    &(SS.sv.e.distance.v)         },
    { 'e',  "Entity.workplane.v",       'x',    &(SS.sv.e.workplane.v)        },
    { 'e',  "Entity.actPoint.x",        'f',    &(SS.sv.e.actPoint.x)         },
    { 'e',  "Entity.actPoint.y",        'f',    &(SS.sv.e.actPoint.y)         },
    { 'e',  "Entity.actPoint.z",        'f',    &(SS.sv.e.actPoint.z)         },
    { 'e',  "Entity.actNormal.w",       'f',    &(SS.sv.e.actNormal.w)        },
    { 'e',  "Entity.actNormal.vx",      'f',    &(SS.sv.e.actNormal.vx)       },
    { 'e',  "Entity.actNormal.vy",      'f',    &(SS.sv.e.actNormal.vy)       },
    { 'e',  "Entity.actNormal.vz",      'f',    &(SS.sv.e.actNormal.vz)       },
    { 'e',  "Entity.actDistance",       'f',    &(SS.sv.e.actDistance)        },
    { 'e',  "Entity.actVisible",        'b',    &(SS.sv.e.actVisible),        },


    { 'c',  "Constraint.h.v",           'x',    &(SS.sv.c.h.v)                },
    { 'c',  "Constraint.type",          'd',    &(SS.sv.c.type)               },
    { 'c',  "Constraint.group.v",       'x',    &(SS.sv.c.group.v)            },
    { 'c',  "Constraint.workplane.v",   'x',    &(SS.sv.c.workplane.v)        },
    { 'c',  "Constraint.valA",          'f',    &(SS.sv.c.valA)               },
    { 'c',  "Constraint.ptA.v",         'x',    &(SS.sv.c.ptA.v)              },
    { 'c',  "Constraint.ptB.v",         'x',    &(SS.sv.c.ptB.v)              },
    { 'c',  "Constraint.entityA.v",     'x',    &(SS.sv.c.entityA.v)          },
    { 'c',  "Constraint.entityB.v",     'x',    &(SS.sv.c.entityB.v)          },
    { 'c',  "Constraint.entityC.v",     'x',    &(SS.sv.c.entityC.v)          },
    { 'c',  "Constraint.entityD.v",     'x',    &(SS.sv.c.entityD.v)          },
    { 'c',  "Constraint.other",         'b',    &(SS.sv.c.other)              },
    { 'c',  "Constraint.other2",        'b',    &(SS.sv.c.other2)             },
    { 'c',  "Constraint.reference",     'b',    &(SS.sv.c.reference)          },
    { 'c',  "Constraint.comment",       'S',    &(SS.sv.c.comment)            },
    { 'c',  "Constraint.disp.offset.x", 'f',    &(SS.sv.c.disp.offset.x)      },
    { 'c',  "Constraint.disp.offset.y", 'f',    &(SS.sv.c.disp.offset.y)      },
    { 'c',  "Constraint.disp.offset.z", 'f',    &(SS.sv.c.disp.offset.z)      },
    { 'c',  "Constraint.disp.style",    'x',    &(SS.sv.c.disp.style)         },

    { 's',  "Style.h.v",                'x',    &(SS.sv.s.h.v)                },
    { 's',  "Style.name",               'S',    &(SS.sv.s.name)               },
    { 's',  "Style.width",              'f',    &(SS.sv.s.width)              },
    { 's',  "Style.widthAs",            'd',    &(SS.sv.s.widthAs)            },
    { 's',  "Style.textHeight",         'f',    &(SS.sv.s.textHeight)         },
    { 's',  "Style.textHeightAs",       'd',    &(SS.sv.s.textHeightAs)       },
    { 's',  "Style.textAngle",          'f',    &(SS.sv.s.textAngle)          },
    { 's',  "Style.textOrigin",         'x',    &(SS.sv.s.textOrigin)         },
    { 's',  "Style.color",              'c',    &(SS.sv.s.color)              },
    { 's',  "Style.fillColor",          'c',    &(SS.sv.s.fillColor)          },
    { 's',  "Style.filled",             'b',    &(SS.sv.s.filled)             },
    { 's',  "Style.visible",            'b',    &(SS.sv.s.visible)            },
    { 's',  "Style.exportable",         'b',    &(SS.sv.s.exportable)         },

    { 0, NULL, 0, NULL }
};

struct SAVEDptr {
    IdList<EntityMap,EntityId> &M() { return *((IdList<EntityMap,EntityId> *)this); }
    /* std::string S; */
    bool      &b() { return *((bool *)this); }
    RgbaColor &c() { return *((RgbaColor *)this); }
    int       &d() { return *((int *)this); }
    double    &f() { return *((double *)this); }
    uint32_t  &x() { return *((uint32_t *)this); }
};

void SolveSpaceUI::SaveUsingTable(int type) {
    int i;
    for(i = 0; SAVED[i].type != 0; i++) {
        if(SAVED[i].type != type) continue;

        int fmt = SAVED[i].fmt;
        SAVEDptr *p = (SAVEDptr *)SAVED[i].ptr;
        // Any items that aren't specified are assumed to be zero
        if(fmt == 'S' && ((std::string*)p)->empty()) continue;
        if(fmt == 'd' && p->d() == 0)             continue;
        if(fmt == 'f' && EXACT(p->f() == 0.0))    continue;
        if(fmt == 'x' && p->x() == 0)             continue;

        fprintf(fh, "%s=", SAVED[i].desc);
        switch(fmt) {
            case 'S': fprintf(fh, "%s",    ((std::string*)p)->c_str()); break;
            case 'b': fprintf(fh, "%d",    p->b() ? 1 : 0);       break;
            case 'c': fprintf(fh, "%08x",  p->c().ToPackedInt()); break;
            case 'd': fprintf(fh, "%d",    p->d());               break;
            case 'f': fprintf(fh, "%.20f", p->f());               break;
            case 'x': fprintf(fh, "%08x",  p->x());               break;

            case 'M': {
                int j;
                fprintf(fh, "{\n");
                for(j = 0; j < p->M().n; j++) {
                    EntityMap *em = &(p->M().elem[j]);
                    fprintf(fh, "    %d %08x %d\n",
                            em->h.v, em->input.v, em->copyNumber);
                }
                fprintf(fh, "}");
                break;
            }

            default: oops();
        }
        fprintf(fh, "\n");
    }
}

bool SolveSpaceUI::SaveToFile(const std::string &filename) {
    // Make sure all the entities are regenerated up to date, since they
    // will be exported. We reload the imported files because that rewrites
    // the impFileRel for our possibly-new filename.
    SS.ScheduleShowTW();
    SS.ReloadAllImported();
    SS.GenerateAll(0, INT_MAX);

    fh = ssfopen(filename, "wb");
    if(!fh) {
        Error("Couldn't write to file '%s'", filename.c_str());
        return false;
    }

    fprintf(fh, "%s\n\n\n", VERSION_STRING);

    int i, j;
    for(i = 0; i < sketch->group.n; i++) {
        sv.g = sketch->group.elem[i];
        SaveUsingTable('g');
        fprintf(fh, "AddGroup\n\n");
    }

    for(i = 0; i < sketch->param.n; i++) {
        sv.p = sketch->param.elem[i];
        SaveUsingTable('p');
        fprintf(fh, "AddParam\n\n");
    }

    for(i = 0; i < sketch->request.n; i++) {
        sv.r = sketch->request.elem[i];
        SaveUsingTable('r');
        fprintf(fh, "AddRequest\n\n");
    }

    for(i = 0; i < sketch->entity.n; i++) {
        (sketch->entity.elem[i]).CalculateNumerical(true);
        sv.e = sketch->entity.elem[i];
        SaveUsingTable('e');
        fprintf(fh, "AddEntity\n\n");
    }

    for(i = 0; i < sketch->constraint.n; i++) {
        sv.c = sketch->constraint.elem[i];
        SaveUsingTable('c');
        fprintf(fh, "AddConstraint\n\n");
    }

    for(i = 0; i < sketch->style.n; i++) {
        sv.s = sketch->style.elem[i];
        if(sv.s.h.v >= Style::FIRST_CUSTOM) {
            SaveUsingTable('s');
            fprintf(fh, "AddStyle\n\n");
        }
    }

    // A group will have either a mesh or a shell, but not both; but the code
    // to print either of those just does nothing if the mesh/shell is empty.

    SMesh *m = &(sketch->group.elem[sketch->group.n-1].runningMesh);
    for(i = 0; i < m->l.n; i++) {
        STriangle *tr = &(m->l.elem[i]);
        fprintf(fh, "Triangle %08x %08x "
                "%.20f %.20f %.20f  %.20f %.20f %.20f  %.20f %.20f %.20f\n",
            tr->meta.face, tr->meta.color.ToPackedInt(),
            CO(tr->a), CO(tr->b), CO(tr->c));
    }

    SShell *s = &(sketch->group.elem[sketch->group.n-1].runningShell);
    SSurface *srf;
    for(srf = s->surface.First(); srf; srf = s->surface.NextAfter(srf)) {
        fprintf(fh, "Surface %08x %08x %08x %d %d\n",
            srf->h.v, srf->color.ToPackedInt(), srf->face, srf->degm, srf->degn);
        for(i = 0; i <= srf->degm; i++) {
            for(j = 0; j <= srf->degn; j++) {
                fprintf(fh, "SCtrl %d %d %.20f %.20f %.20f Weight %20.20f\n",
                    i, j, CO(srf->ctrl[i][j]), srf->weight[i][j]);
            }
        }

        STrimBy *stb;
        for(stb = srf->trim.First(); stb; stb = srf->trim.NextAfter(stb)) {
            fprintf(fh, "TrimBy %08x %d %.20f %.20f %.20f  %.20f %.20f %.20f\n",
                stb->curve.v, stb->backwards ? 1 : 0,
                CO(stb->start), CO(stb->finish));
        }

        fprintf(fh, "AddSurface\n");
    }
    SCurve *sc;
    for(sc = s->curve.First(); sc; sc = s->curve.NextAfter(sc)) {
        fprintf(fh, "Curve %08x %d %d %08x %08x\n",
            sc->h.v,
            sc->isExact ? 1 : 0, sc->exact.deg,
            sc->surfA.v, sc->surfB.v);

        if(sc->isExact) {
            for(i = 0; i <= sc->exact.deg; i++) {
                fprintf(fh, "CCtrl %d %.20f %.20f %.20f Weight %.20f\n",
                    i, CO(sc->exact.ctrl[i]), sc->exact.weight[i]);
            }
        }
        SCurvePt *scpt;
        for(scpt = sc->pts.First(); scpt; scpt = sc->pts.NextAfter(scpt)) {
            fprintf(fh, "CurvePt %d %.20f %.20f %.20f\n",
                scpt->vertex ? 1 : 0, CO(scpt->p));
        }

        fprintf(fh, "AddCurve\n");
    }

    fclose(fh);

    return true;
}

void SolveSpaceUI::LoadUsingTable(char *key, char *val) {
    int i;
    for(i = 0; SAVED[i].type != 0; i++) {
        if(strcmp(SAVED[i].desc, key)==0) {
            SAVEDptr *p = (SAVEDptr *)SAVED[i].ptr;
            unsigned int u = 0;
            switch(SAVED[i].fmt) {
                case 'S': (*(std::string*)p) = val; break;
                case 'b': p->b() = (atoi(val) != 0); break;
                case 'd': p->d() = atoi(val);        break;
                case 'f': p->f() = atof(val);        break;
                case 'x': sscanf(val, "%x", &u); p->x()= u; break;

                case 'c':
                    sscanf(val, "%x", &u);
                    p->c() = RgbaColor::FromPackedInt(u);
                    break;

                case 'P':
                    *((std::string*)p) = val;
                    break;

                case 'M': {
                    // Don't clear this list! When the group gets added, it
                    // makes a shallow copy, so that would result in us
                    // freeing memory that we want to keep around. Just
                    // zero it out so that new memory is allocated.
                    p->M() = {};
                    for(;;) {
                        EntityMap em;
                        char line2[1024];
                        if (fgets(line2, (int)sizeof(line2), fh) == NULL)
                            break;
                        if(sscanf(line2, "%d %x %d", &(em.h.v), &(em.input.v),
                                                     &(em.copyNumber)) == 3)
                        {
                            p->M().Add(&em);
                        } else {
                            break;
                        }
                    }
                    break;
                }

                default: oops();
            }
            break;
        }
    }
    if(SAVED[i].type == 0) {
        fileLoadError = true;
    }
}

bool SolveSpaceUI::LoadFromFile(const std::string &filename) {
    allConsistent = false;
    fileLoadError = false;

    fh = ssfopen(filename, "rb");
    if(!fh) {
        Error("Couldn't read from file '%s'", filename.c_str());
        return false;
    }

    ClearExisting();

    sv = {sketch};
    sv.g.scale = 1; // default is 1, not 0; so legacy files need this

    char line[1024];
    while(fgets(line, (int)sizeof(line), fh)) {
        char *s = strchr(line, '\n');
        if(s) *s = '\0';
        // We should never get files with \r characters in them, but mailers
        // will sometimes mangle attachments.
        s = strchr(line, '\r');
        if(s) *s = '\0';

        if(*line == '\0') continue;

        char *e = strchr(line, '=');
        if(e) {
            *e = '\0';
            char *key = line, *val = e+1;
            LoadUsingTable(key, val);
        } else if(strcmp(line, "AddGroup")==0) {
            sketch->group.Add(&(sv.g));
            sv.g = Group{ sv.sketch };
            sv.g.scale = 1; // default is 1, not 0; so legacy files need this
        } else if(strcmp(line, "AddParam")==0) {
            // params are regenerated, but we want to preload the values
            // for initial guesses
            sketch->param.Add(&(sv.p));
            sv.p = {};
        } else if(strcmp(line, "AddEntity")==0) {
            // entities are regenerated
        } else if(strcmp(line, "AddRequest")==0) {
            sketch->request.Add(&(sv.r));
            sv.r = Request{ sv.sketch };
        } else if(strcmp(line, "AddConstraint")==0) {
            sketch->constraint.Add(&(sv.c));
            sv.c = Constraint{ sv.sketch };
        } else if(strcmp(line, "AddStyle")==0) {
            sketch->style.Add(&(sv.s));
            sv.s = {};
        } else if(strcmp(line, VERSION_STRING)==0) {
            // do nothing, version string
        } else if(StrStartsWith(line, "Triangle ")      ||
                  StrStartsWith(line, "Surface ")       ||
                  StrStartsWith(line, "SCtrl ")         ||
                  StrStartsWith(line, "TrimBy ")        ||
                  StrStartsWith(line, "Curve ")         ||
                  StrStartsWith(line, "CCtrl ")         ||
                  StrStartsWith(line, "CurvePt ")       ||
                  strcmp(line, "AddSurface")==0         ||
                  strcmp(line, "AddCurve")==0)
        {
            // ignore the mesh or shell, since we regenerate that
        } else {
            fileLoadError = true;
        }
    }

    fclose(fh);

    if(fileLoadError) {
        Error("Unrecognized data in file. This file may be corrupt, or "
              "from a new version of the program.");
        // At least leave the program in a non-crashing state.
        if(sketch->group.n == 0) {
            NewFile();
        }
    }

    return true;
}

bool SolveSpaceUI::LoadEntitiesFromFile(const std::string &filename, EntityList *le,
                                        SMesh *m, SShell *sh)
{
    SSurface srf {};
    SCurve crv {};

    fh = ssfopen(filename, "rb");
    if(!fh) return false;

    le->Clear();
    sv = {sketch};

    char line[1024];
    while(fgets(line, (int)sizeof(line), fh)) {
        char *s = strchr(line, '\n');
        if(s) *s = '\0';
        // We should never get files with \r characters in them, but mailers
        // will sometimes mangle attachments.
        s = strchr(line, '\r');
        if(s) *s = '\0';

        if(*line == '\0') continue;

        char *e = strchr(line, '=');
        if(e) {
            *e = '\0';
            char *key = line, *val = e+1;
            LoadUsingTable(key, val);
        } else if(strcmp(line, "AddGroup")==0) {
            // Don't leak memory; these get allocated whether we want them
            // or not.
            sv.g.remap.Clear();
        } else if(strcmp(line, "AddParam")==0) {

        } else if(strcmp(line, "AddEntity")==0) {
            le->Add(&(sv.e));
            sv.e = Entity{ sv.sketch };
        } else if(strcmp(line, "AddRequest")==0) {

        } else if(strcmp(line, "AddConstraint")==0) {

        } else if(strcmp(line, "AddStyle")==0) {

        } else if(strcmp(line, VERSION_STRING)==0) {

        } else if(StrStartsWith(line, "Triangle ")) {
            STriangle tr {};
            unsigned int rgba = 0;
            if(sscanf(line, "Triangle %x %x  "
                             "%lf %lf %lf  %lf %lf %lf  %lf %lf %lf",
                &(tr.meta.face), &rgba,
                &(tr.a.x), &(tr.a.y), &(tr.a.z),
                &(tr.b.x), &(tr.b.y), &(tr.b.z),
                &(tr.c.x), &(tr.c.y), &(tr.c.z)) != 11) {
                oops();
            }
            tr.meta.color = RgbaColor::FromPackedInt((uint32_t)rgba);
            m->AddTriangle(&tr);
        } else if(StrStartsWith(line, "Surface ")) {
            unsigned int rgba = 0;
            if(sscanf(line, "Surface %x %x %x %d %d",
                &(srf.h.v), &rgba, &(srf.face),
                &(srf.degm), &(srf.degn)) != 5) {
                oops();
            }
            srf.color = RgbaColor::FromPackedInt((uint32_t)rgba);
        } else if(StrStartsWith(line, "SCtrl ")) {
            int i, j;
            Vector c;
            double w;
            if(sscanf(line, "SCtrl %d %d %lf %lf %lf Weight %lf",
                                &i, &j, &(c.x), &(c.y), &(c.z), &w) != 6)
            {
                oops();
            }
            srf.ctrl[i][j] = c;
            srf.weight[i][j] = w;
        } else if(StrStartsWith(line, "TrimBy ")) {
            STrimBy stb {};
            int backwards;
            if(sscanf(line, "TrimBy %x %d  %lf %lf %lf  %lf %lf %lf",
                &(stb.curve.v), &backwards,
                &(stb.start.x), &(stb.start.y), &(stb.start.z),
                &(stb.finish.x), &(stb.finish.y), &(stb.finish.z)) != 8)
            {
                oops();
            }
            stb.backwards = (backwards != 0);
            srf.trim.Add(&stb);
        } else if(strcmp(line, "AddSurface")==0) {
            sh->surface.Add(&srf);
            srf = {};
        } else if(StrStartsWith(line, "Curve ")) {
            int isExact;
            if(sscanf(line, "Curve %x %d %d %x %x",
                &(crv.h.v),
                &(isExact),
                &(crv.exact.deg),
                &(crv.surfA.v), &(crv.surfB.v)) != 5)
            {
                oops();
            }
            crv.isExact = (isExact != 0);
        } else if(StrStartsWith(line, "CCtrl ")) {
            int i;
            Vector c;
            double w;
            if(sscanf(line, "CCtrl %d %lf %lf %lf Weight %lf",
                                &i, &(c.x), &(c.y), &(c.z), &w) != 5)
            {
                oops();
            }
            crv.exact.ctrl[i] = c;
            crv.exact.weight[i] = w;
        } else if(StrStartsWith(line, "CurvePt ")) {
            SCurvePt scpt;
            int vertex;
            if(sscanf(line, "CurvePt %d %lf %lf %lf",
                &vertex,
                &(scpt.p.x), &(scpt.p.y), &(scpt.p.z)) != 4)
            {
                oops();
            }
            scpt.vertex = (vertex != 0);
            crv.pts.Add(&scpt);
        } else if(strcmp(line, "AddCurve")==0) {
            sh->curve.Add(&crv);
            crv = {};
        } else {
            oops();
        }
    }

    fclose(fh);
    return true;
}

//-----------------------------------------------------------------------------
// Handling of the relative-absolute path transformations for imports
//-----------------------------------------------------------------------------
static std::vector<std::string> Split(const std::string &haystack, const std::string &needle)
{
    std::vector<std::string> result;

    size_t oldpos = 0, pos = 0;
    while(true) {
        oldpos = pos;
        pos = haystack.find(needle, pos);
        if(pos == std::string::npos) break;
        result.push_back(haystack.substr(oldpos, pos - oldpos));
        pos += needle.length();
    }

    if(oldpos != haystack.length() - 1)
        result.push_back(haystack.substr(oldpos));

    return result;
}

static std::string Join(const std::vector<std::string> &parts, const std::string &separator)
{
    bool first = true;
    std::string result;
    for(auto &part : parts) {
        if(!first) result += separator;
        result += part;
        first = false;
    }
    return result;
}

static bool PlatformPathEqual(const std::string &a, const std::string &b)
{
    // Case-sensitivity is actually per-volume on both Windows and OS X,
    // but it is extremely tedious to implement and test for little benefit.
#if defined(WIN32)
    std::wstring wa = Widen(a), wb = Widen(b);
    return std::equal(wa.begin(), wa.end(), wb.begin(), /*wb.end(),*/
                [](wchar_t wca, wchar_t wcb) { return towlower(wca) == towlower(wcb); });
#elif defined(__APPLE__)
    return !strcasecmp(a.c_str(), b.c_str());
#else
    return a == b;
#endif
}

static std::string MakePathRelative(const std::string &base, const std::string &path)
{
    std::vector<std::string> baseParts = Split(base, PATH_SEP),
                             pathParts = Split(path, PATH_SEP),
                             resultParts;
    baseParts.pop_back();

    int common;
    for(common = 0; common < baseParts.size() && common < pathParts.size(); common++) {
        if(!PlatformPathEqual(baseParts[common], pathParts[common]))
            break;
    }

    for(int i = common; i < baseParts.size(); i++)
        resultParts.push_back("..");

    resultParts.insert(resultParts.end(),
                       pathParts.begin() + common, pathParts.end());

    return Join(resultParts, PATH_SEP);
}

static std::string MakePathAbsolute(const std::string &base, const std::string &path)
{
    std::vector<std::string> resultParts = Split(base, PATH_SEP),
                             pathParts = Split(path, PATH_SEP);
    resultParts.pop_back();

    for(auto &part : pathParts) {
        if(part == ".") {
            /* do nothing */
        } else if(part == "..") {
            if(resultParts.empty()) oops();
            resultParts.pop_back();
        } else {
            resultParts.push_back(part);
        }
    }

    return Join(resultParts, PATH_SEP);
}

static void PathSepNormalize(std::string &filename)
{
    for(int i = 0; i < filename.length(); i++) {
        if(filename[i] == '\\')
            filename[i] = '/';
    }
}

static std::string PathSepPlatformToUNIX(const std::string &filename)
{
#if defined(WIN32)
    std::string result = filename;
    for(int i = 0; i < result.length(); i++) {
        if(result[i] == '\\')
            result[i] = '/';
    }
    return result;
#else
    return filename;
#endif
}

static std::string PathSepUNIXToPlatform(const std::string &filename)
{
#if defined(WIN32)
    std::string result = filename;
    for(int i = 0; i < result.length(); i++) {
        if(result[i] == '/')
            result[i] = '\\';
    }
    return result;
#else
    return filename;
#endif
}

void SolveSpaceUI::ReloadAllImported(void)
{
    allConsistent = false;

    int i;
    for(i = 0; i < sketch->group.n; i++) {
        Group *g = &(sketch->group.elem[i]);
        if(g->type != Group::IMPORTED) continue;

        if(isalpha(g->impFile[0]) && g->impFile[1] == ':') {
            // Make sure that g->impFileRel always contains a relative path
            // in an UNIX format, even after we load an old file which had
            // the path in Windows format
            PathSepNormalize(g->impFileRel);
        }

        g->impEntity.Clear();
        g->impMesh.Clear();
        g->impShell.Clear();

        FILE *test = ssfopen(g->impFile, "rb");
        if(test) {
            fclose(test); // okay, exists
        } else {
            // It doesn't exist. Perhaps the entire tree has moved, and we
            // can use the relative filename to get us back.
            if(!SS.saveFile.empty()) {
                std::string rel = PathSepUNIXToPlatform(g->impFileRel);
                std::string fromRel = MakePathAbsolute(SS.saveFile, rel);
                test = ssfopen(fromRel, "rb");
                if(test) {
                    fclose(test);
                    // It worked, this is our new absolute path
                    g->impFile = fromRel;
                }
            }
        }

        if(LoadEntitiesFromFile(g->impFile, &(g->impEntity), &(g->impMesh), &(g->impShell)))
        {
            if(!SS.saveFile.empty()) {
                // Record the imported file's name relative to our filename;
                // if the entire tree moves, then everything will still work
                std::string rel = MakePathRelative(SS.saveFile, g->impFile);
                g->impFileRel = PathSepPlatformToUNIX(rel);
            } else {
                // We're not yet saved, so can't make it absolute.
                // This will only be used for display purposes, as SS.saveFile
                // is always nonempty when we are actually writing anything.
                g->impFileRel = g->impFile;
            }
        } else {
            Error("Failed to load imported file '%s'", g->impFile.c_str());
        }
    }
}

