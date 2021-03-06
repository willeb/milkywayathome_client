/* Copyright 2010 Matthew Arsenault, Travis Desell, Dave Przybylo,
Nathan Cole, Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik
Magdon-Ismail and Rensselaer Polytechnic Institute.

This file is part of Milkway@Home.

Milkyway@Home is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Milkyway@Home is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <assert.h>
#include "code.h"
#include "json_params.h"
#include "defs.h"

/* TODO: wuh wuh windows */
#include <unistd.h>

static void get_params_from_json(NBodyCtx* ctx, InitialConditions* ic, json_object* fileObj);
static bool warn_extra_params(json_object* obj, const char* grpName);
static void readParameterGroup(const Parameter*, json_object*, const Parameter*);


static void processPotential(Potential* p)
{


}

static void processHalo(Halo* h)
{


}

static void processModel(DwarfModel* mod)
{
    const int r0 = mod->scale_radius;

    if (isnan(mod->time_dwarf) && isnan(mod->time_orbit))
        fail("At least one of the evolution times must be specified for the dwarf model\n");

    switch (mod->type)
    {
        case DwarfModelPlummer:
            /* If not set, and no default, it's calculated based on
             * other parameters. */
            if (isnan(mod->eps))
                mod->eps = r0 / (10 * sqrt((real) mod->nbody));

            if (isnan(mod->timestep))
                mod->timestep = sqr(1/10.0) * sqrt(  (PI_4_3 * cube(r0)) / mod->mass);

            break;

        case DwarfModelKing:
        case DwarfModelDehnen:
        default:
            fail("Unhandled model type: %d\n", mod->type);
    }

    if (isnan(mod->time_orbit))
        mod->time_orbit = mod->timestep / 2.0;

    /* if (isnan(mod->time_dwarf))
           ?????;
    */

}

static void processInitialConditions(InitialConditions* ic)
{
    real lrad, brad;
    if (!ic->useGalC)
    {
        /* convert coordinates depending on system used */
        lrad = d2r( L(ic) );
        brad = d2r( B(ic) );

        X(ic) = R(ic) * cos(lrad) * cos(brad) - ic->sunGCDist;
        Y(ic) = R(ic) * sin(lrad) * cos(brad);
        Z(ic) = R(ic) * sin(brad);
    }

}

/* Calculate needed parameters from whatever we read in */
/* TODO: I'm dissatisfied with having to throw these checks here at
 * the end */
static void postProcess(NBodyCtx* ctx)
{
    processPotential(&ctx->pot);
    processModel(&ctx->model);


    /* These other pieces are dependent on the others being set up
     * first */
    ctx->freqout = ctx->freq = inv(ctx->model.timestep);

}


/* Read command line arguments and initialize the context and state */
void initNBody(NBodyCtx* ctx, InitialConditions* ic, const int argc, const char** argv)
{
    poptContext context;
    int o;
    static char* inputFile = NULL;        /* input JSON file */
    static char* inputStr  = NULL;        /* a string of JSON to use directly */
    static char* outFileName = NULL;   /* output file name */
    static json_object* obj;

    /* FIXME: There's a small leak of the inputFile from use of
       poptGetNextOpt().  Some mailing list post suggestst that this
       is some kind of semi-intended bug to work around something or
       other while maintaining ABI compatability */
    static const struct poptOption options[] =
    {
        {
            "input-file", 'f',
            POPT_ARG_STRING, &inputFile,
            0, "Input file to read", NULL
        },

        {
            "output-file", 'o',
            POPT_ARG_STRING, &outFileName,
            0, "Output file", NULL
        },

        {
            "input-string", 's',
            POPT_ARG_STRING, &inputStr,
            0, "Input given as string", NULL
        },

        POPT_AUTOHELP

        { NULL, 0, 0, NULL, 0, NULL, NULL }
    };

    context = poptGetContext(argv[0],
                             argc,
                             argv,
                             options,
                             POPT_CONTEXT_POSIXMEHARDER);

    if (argc < 2)
    {
        poptPrintUsage(context, stderr, 0);
        poptFreeContext(context);
        exit(EXIT_FAILURE);
    }

    while ( ( o = poptGetNextOpt(context)) >= 0 );

    /* Check for invalid options, and must have one of input file or input string */
    if (     o < -1
         ||  (inputFile && inputStr)
         || !(inputFile || inputStr))
    {
        poptPrintHelp(context, stderr, 0);
        free(inputFile);
        free(inputStr);
        poptFreeContext(context);
        exit(EXIT_FAILURE);
    }

    poptFreeContext(context);

    if (inputFile)
    {
        /* check if we can read the file, so we can fail saying that
         * and not be left to guessing if it's that or a parse
         * error */
        if (access(inputFile, R_OK) < 0)
        {
            perror("Failed to read input file.");
            free(inputFile);
            exit(EXIT_FAILURE);
        }

        /* The lack of parse errors from json-c is unfortunate.
           TODO: If we use the tokener directly, can get them.
         */
        obj = json_object_from_file(inputFile);
        if (is_error(obj))
        {
            fprintf(stderr,
                    "Parse error in file '%s'\n",
                    inputFile);
            free(inputFile);
            exit(EXIT_FAILURE);
        }
        free(inputFile);
    }
    else
    {
        obj = json_tokener_parse(inputStr);
        free(inputStr);
        if (is_error(obj))
            fail("Failed to parse given string\n");
    }

    ctx->outfilename = outFileName;

    get_params_from_json(ctx, ic, obj);

    printContext(ctx);

}

/* also works for json_object_type */
static const char* showNBodyType(nbody_type bt)
{
    switch (bt)
    {
        case nbody_type_null:
            return "null";
        case nbody_type_boolean:
            return "bool";
        case nbody_type_double:
            return "double";
        case nbody_type_int:
            return "int";
        case nbody_type_object:
            return "object";
        case nbody_type_array:
            return "array";
        case nbody_type_vector:
            return "vector";
        case nbody_type_string:
            return "string";
        case nbody_type_enum:
            return "enum";
        case nbody_type_group:
            return "group";
        case nbody_type_group_item:
            return "group_item";
        default:
            fail("Trying to show unknown nbody_type %d\n", bt);
    }
}

/* Reads a name of the criterion into the C value, with name str */
static criterion_t readCriterion(const char* str)
{
    if (!strcasecmp(str, "bh86"))
        return BH86;
    else if (!strcasecmp(str, "sw93"))
        return SW93;
    else
        fail("Invalid model %s: Model options are either 'bh86' or 'sw93'\n", str);
}

void printParameter(Parameter* p)
{
    if (p)
    {
        printf("parameter = {\n"
               "  name  = %s\n"
               "  type  = %d\n"
               "  param = %p\n"
               "  dflt  = %p\n"
               "  conv  = %p\n"
               "  uniq  = %d\n"
               "  pms   = %p\n"
               "};\n",
               p->name,
               p->type,
               p->param,
               p->dflt,
               p->conv,
               p->unique,
               p->parameters);
    }
    else
    {
        printf("<NULL PARAMETER>\n");
    }
}

/* Read a set of related parameters, e.g. the main NBodyCtx.
   If the unique flag is set, it's only valid to have one of the items in the group.
   Otherwise, it tries to use all of the parameters, and warns when extra elements are found.
   Fails if it fails to find a parameter that isn't defaultable.
 */
static void readParameterGroup(const Parameter* g,      /* The set of parameters */
                               json_object* hdr,        /* The object of the group */
                               const Parameter* parent) /* non-null if within another group */
{
    const Parameter* p;
    const Parameter* q;
    bool useDflt, defaultable = FALSE;
    json_object* obj;
    const char* pname;
    bool unique;
    bool found = FALSE, done = FALSE, readError = FALSE;
    generic_enum_t* group_type;
    array_list* arr;
    json_object* tmp;
    int i, arrLen;

    if (parent)
    {
        pname = parent->name;     /* Name of the group we're in */
        unique = parent->unique;  /* Expect one, or try to take all of them*/
        group_type = (generic_enum_t*) parent->param;
    }
    else
    {
        pname = "<root>";       /* at the root of the configuration */
        unique = FALSE;
        group_type = NULL;
    }

    assert(g);
    assert(hdr);

    p = g;

    /* CHECKME: Handling of defaultable, cases in unique? */
    while (p->name && !done && !readError)
    {
        defaultable = (p->dflt != NULL);
        useDflt = FALSE;
        found = FALSE;

        obj = json_object_object_get(hdr, p->name);
        if (!obj)
        {
            if (unique)  /* We get another chance */
            {
                ++p;
                continue;
            }

            if (defaultable)
                useDflt = TRUE;
            else
            {
                fprintf(stderr,
                        "Failed to find or got 'null' for required key '%s' in '%s'\n",
                        p->name,
                        pname);
                readError = TRUE;
                break;  /* abandon the loop and print useful debugging */
            }
        }
        else
        {
            found = TRUE;
            if (unique)    /* Only want one parameter, everything else is extra and a warning */
                done = TRUE;

            /* TODO: Better type checking might be nice. Mostly now relies
             * on not screwing up the tables. */
            switch (p->type)
            {
                case nbody_type_double:
                    /* json_type_int and double are OK for numbers. i.e. you can leave off the decimal.
                       We don't want the other conversions, which just give you 0.0 for anything else.
                    */
                    if (   json_object_is_type(obj, json_type_double)
                           || json_object_is_type(obj, json_type_int))
                    {
                        *((real*) p->param) =
                            useDflt ? *((real*) p->dflt) : (real) json_object_get_double(obj);
                    }
                    else
                    {
                        fprintf(stderr,
                                "Error: expected number for '%s' in '%s', but got %s\n",
                                p->name,
                                pname,
                                showNBodyType(p->type));
                        readError = TRUE;
                    }
                    break;

                case nbody_type_int:
                    /* I don't think any of the conversions are acceptable */
                    if (json_object_is_type(obj, json_type_int))
                        *((int*) p->param) = useDflt ? *((int*) p->dflt) : json_object_get_int(obj);
                    else
                    {
                        fprintf(stderr,
                                "Error: expected type int for '%s' in '%s', but got %s\n",
                                p->name,
                                pname,
                                showNBodyType(json_object_get_type(obj)));
                        readError = TRUE;
                    }
                            break;

                case nbody_type_boolean:  /* CHECKME: Size */
                    *((bool*) p->param) = useDflt ? *((int*) p->dflt) : json_object_get_boolean(obj);
                    break;

                case nbody_type_string:
                    /* the json_object has ownership of the string so we need to copy it */
                    *((char**) p->param) =
                        useDflt ? *((char**) p->dflt) : strdup(json_object_get_string(obj));
                    break;

                case nbody_type_vector:
                    assert(json_object_is_type(obj, json_type_array));
                    arr = json_object_get_array(obj);
                    arrLen = json_object_array_length(obj);
                    if (arrLen != 3)
                    {
                        fail("Got %d items for array '%s' in '%s', expected 3\n",
                             arrLen,
                             p->name,
                             pname);
                    }

                    for ( i = 0; i < 3; ++i )
                    {
                        tmp = (json_object*) array_list_get_idx(arr, i);

                        if (    !json_object_is_type(tmp, json_type_double)
                             && !json_object_is_type(tmp, json_type_int))
                        {
                            fail("Got unexpected type '%s' in position %d "
                                 "of key '%s' in '%s', expected number.\n",
                                 showNBodyType(json_object_get_type(tmp)),
                                 i,
                                 p->name,
                                 pname);
                        }

                        ((real*) p->param)[i] = json_object_get_double(tmp);
                    }
                    break;

                case nbody_type_group_item:
                    if (p->dflt)
                        *group_type = *((generic_enum_t*) p->dflt);
                    else
                    {
                        fail("Expected nbody_type_group_item for "
                             "'%s' in '%s', but no enum value set\n", p->name, pname);
                    }

                    /* fall through to nbody_type_object */
                case nbody_type_group:
                case nbody_type_object:
                    readParameterGroup(p->parameters, obj, p);
                    break;
                case nbody_type_enum:
                    /* This is actually a json_type_string, which we read
                     * into an enum, or take a default value */
                    if (useDflt)
                    {
                        *((generic_enum_t*) p->param) = *((generic_enum_t*) p->dflt);
                        break;
                    }

                    if (p->conv)
                        *((generic_enum_t*) p->param) = p->conv(json_object_get_string(obj));
                    else
                        fail("Error: read function not set for enum '%s'\n", p->name);

                    break;
                default:
                    fail("Unhandled parameter type %d for key '%s' in '%s'\n",
                         p->type,
                         p->name,
                         pname);
            }

            /* Explicitly delete it so we can check for extra stuff */
            json_object_object_del(hdr, p->name);
        }
        ++p;
    }

    /* Skip the extra parameter warning if there was an error, since
     * abandoning the loop leaves lots of stuff in it */
    if (!readError)
        warn_extra_params(hdr, pname);

    /* Report what was expected in more detail */
    if ((!found || readError) && !defaultable)
    {
        fprintf(stderr, "Failed to find required item of correct type in group '%s'\n", pname);

        if (unique)
            fprintf(stderr, "\tExpected to find one of the following:\n");
        else
            fprintf(stderr, "\tFields are:\n");

        q = g;
        while (q->name)
        {
            fprintf(stderr, "\t\t%s (%s)", q->name, showNBodyType(q->type));

            if (q->dflt && !unique)
                fprintf(stderr, "  (optional)");
            fprintf(stderr, ",\n");
            ++q;
        }

        exit(EXIT_FAILURE);
    }
}


/* Iterate through remaining keys in obj to provide useful warnings of
 * unknown parameters in the file. Returns true if any found. */
static bool warn_extra_params(json_object* obj, const char* grpName)
{
    bool haveExtra = FALSE;

    json_object_object_foreach(obj,key,val)
    {
        haveExtra = TRUE;
        fprintf(stderr,
                "Warning: In group '%s': Unknown field '%s': '%s'\n",
                grpName,
                key,
                json_object_to_json_string(val));
    }

    return haveExtra;
}


/* CHECKME: Assumption that all enums are the same size, size of an
 * int, which I think is always true on all compilers, but I'm
 * probably wrong. */

/* TODO: Could use hash table more directly and avoid all the extra
 * lookups, but it probably doesn't matter. */


/* Read the parameters from the top level json object into ctx. It
 * destroys the object in the process. */
void get_params_from_json(NBodyCtx* ctx, InitialConditions* ic, json_object* fileObj)
{
    /* Constants used for defaulting. Each field only used if
     * specified in the actual parameter tables. */
    const NBodyCtx defaultCtx =
        {
            .pot = EMPTY_POTENTIAL,
            .tstop = NAN,
            .dtout = NAN,
            .freq = NAN,
            .freqout = NAN,
            .usequad = TRUE,
            .allowIncest = FALSE,
            .criterion = BH86,
            .theta = 0.0,
            .seed = 0,
            .outfile = NULL,
            .outfilename = NULL,
            .headline = NULL
        };

    const Tree defaultTree =
        {
            .rsize = 4.0
        };

    /* Spherical potential options */
    const Parameter sphericalParams[] =
        {
            DBL_PARAM("mass",     &ctx->pot.sphere[0].mass),
            DBL_PARAM("r0-scale", &ctx->pot.sphere[0].scale),
            NULLPARAMETER
        };

    /* TODO: Different spherical potentials, more spherical potentials */
    /* This is maybe redundant and annoying, (spherical in spherical),
     * but to be consistent with the others. Also part of the way to
     * having multiple spherical potentials. */
    const spherical_t sphT = SphericalPotential;
    const Parameter sphericalOptions[] =
        {
            GROUP_PARAM_ITEM("sphere", &sphT,  sphericalParams),
            NULLPARAMETER
        };

    /* Disk potential options */
    const Parameter miaymotoParams[] =
        {
            DBL_PARAM("mass",         &ctx->pot.disk.mass),
            DBL_PARAM("scale-length", &ctx->pot.disk.scale_length),
            DBL_PARAM("scale-height", &ctx->pot.disk.scale_height),
            NULLPARAMETER
        };

    const Parameter exponentialParams[] =
        {
            DBL_PARAM("mass",         &ctx->pot.disk.mass),
            DBL_PARAM("scale-length", &ctx->pot.disk.scale_length),
            NULLPARAMETER
        };

    /* Can't take the address of a literal. Also using pointers to
     * guarantee a consistent size. */
    const disk_t mnd = MiaymotoNagaiDisk, expd = ExponentialDisk;
    const Parameter diskOptions[] =
        {
            GROUP_PARAM_ITEM("miyamoto-nagai", &mnd,  miaymotoParams),
            GROUP_PARAM_ITEM("exponential",    &expd, exponentialParams),
            NULLPARAMETER
        };

    /* Halo options */
    const Parameter nfwParams[] =
        {
            DBL_PARAM("scale-length", &ctx->pot.halo.scale_length),
            DBL_PARAM("vhalo",        &ctx->pot.halo.vhalo),
            NULLPARAMETER
        };

    const Parameter logarithmicParams[] =
        {
            DBL_PARAM("vhalo",        &ctx->pot.halo.vhalo),
            DBL_PARAM("z-flattening", &ctx->pot.halo.flattenZ),
            DBL_PARAM("scale-length", &ctx->pot.halo.scale_length),
            NULLPARAMETER
        };

    const Parameter triaxialParams[] =
        {
            DBL_PARAM("vhalo",          &ctx->pot.halo.vhalo),
            DBL_PARAM("scale-length",   &ctx->pot.halo.scale_length),
            DBL_PARAM("x-flattening",   &ctx->pot.halo.flattenX),
            DBL_PARAM("y-flattening",   &ctx->pot.halo.flattenY),
            DBL_PARAM("z-flattening",   &ctx->pot.halo.flattenZ),
            DBL_PARAM("triaxial angle", &ctx->pot.halo.triaxAngle),
            NULLPARAMETER
        };

    const halo_t logHaloT = LogarithmicHalo, nfwHaloT = NFWHalo, triHaloT = TriaxialHalo;
    const Parameter haloOptions[] =
        {
            GROUP_PARAM_ITEM("nfw",         &logHaloT, nfwParams),
            GROUP_PARAM_ITEM("logarithmic", &nfwHaloT, logarithmicParams),
            GROUP_PARAM_ITEM("triaxial",    &triHaloT, triaxialParams),
            NULLPARAMETER
        };

    const Parameter potentialItems[] =
        {
            GROUP_PARAM("disk",      &ctx->pot.disk.type,      diskOptions),
            GROUP_PARAM("halo",      &ctx->pot.halo.type,      haloOptions),
            GROUP_PARAM("spherical", &ctx->pot.sphere[0].type, sphericalOptions),
            NULLPARAMETER
        };

    /* The current different dwarf models all use the same parameters */
    const real nanN = NAN;
    const Parameter dwarfModelParams[] =
        {
            DBL_PARAM("mass",         &ctx->model.mass),
            INT_PARAM("nbody",        &ctx->model.nbody),
            DBL_PARAM("scale-radius", &ctx->model.scale_radius),
            DBL_PARAM_DFLT("eps",     &ctx->model.eps, &nanN),
            DBL_PARAM_DFLT("time-dwarf",   &ctx->model.time_dwarf, &nanN),
            DBL_PARAM_DFLT("time-orbit",   &ctx->model.time_orbit, &nanN),
            NULLPARAMETER
        };

    const dwarf_model_t plummerT = DwarfModelPlummer, kingT = DwarfModelKing, dehnenT = DwarfModelDehnen;
    const Parameter dwarfModelOptions[] =
        {
            GROUP_PARAM_ITEM("plummer", &plummerT, dwarfModelParams),
            GROUP_PARAM_ITEM("king",    &kingT,    dwarfModelParams),
            GROUP_PARAM_ITEM("dehnen",  &dehnenT,  dwarfModelParams),
            NULLPARAMETER
        };

    /* Must be null terminated arrays */
    const Parameter nbodyCtxParams[] =
        {
            STR_PARAM("headline",                    &ctx->headline),
            INT_PARAM("seed",                        &ctx->seed),
            BOOL_PARAM("use-quadrupole-corrections", &ctx->usequad),

            BOOL_PARAM_DFLT("allow-incest",          &ctx->allowIncest, &defaultCtx.allowIncest),
            DBL_PARAM_DFLT("accuracy-parameter",     &ctx->theta, &defaultCtx.theta),
            ENUM_PARAM("criterion",                  &ctx->criterion, (ReadEnum) readCriterion),
            OBJ_PARAM("potential", potentialItems),
            GROUP_PARAM("dwarf-model",               &ctx->model.type, dwarfModelOptions),
            NULLPARAMETER
        };

    const Parameter treeParams[] =
        {
            DBL_PARAM_DFLT("rsize", &t.rsize, &defaultTree.rsize),
            NULLPARAMETER
        };

    const Parameter initialConditions[] =
        {
            BOOL_PARAM("useGalC", &ic->useGalC),
            VEC_PARAM("position", &ic->velocity),
            VEC_PARAM("velocity", &ic->position),
            NULLPARAMETER
        };

    const Parameter parameters[] =
        {
            OBJ_PARAM("nbody-context", nbodyCtxParams),
            OBJ_PARAM("tree",          treeParams),
            OBJ_PARAM("initial-conditions", initialConditions),
            NULLPARAMETER
        };

    json_object* hdr;

    /* Check that this is actually one of our files */
    if (   !json_object_is_type(fileObj, nbody_type_object)
        || !(hdr = json_object_object_get(fileObj, "nbody-parameters-file")))
    {
        fail("Parameters not in expected format.\n");
    }

    /* loop through table of accepted sets of parameters */
    readParameterGroup(parameters, hdr, NULL);

    /* deref the top level object should take care of freeing whatever's left */
    json_object_put(fileObj);

    printInitialConditions(ic);

    printContext(ctx);
    postProcess(ctx);
    printf("\n\nPost processed:\n\n");
    processInitialConditions(ic);
    printInitialConditions(ic);

}


