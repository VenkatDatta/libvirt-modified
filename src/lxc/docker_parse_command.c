#include <config.h>
#include <stdio.h>
#include "util/viralloc.h"
#include "util/virfile.h"
#include "util/virstring.h"
#include "util/virconf.h"
#include "virerror.h"
#include "virlog.h"
#include "conf/domain_conf.h"
#include "docker_parse_command.h"
#include "c-ctype.h"
#include "secret_conf.h"
#include "json.h"
#define VIR_FROM_THIS VIR_FROM_LXC
VIR_LOG_INIT("lxc.docker_parse_command");

static int jsoneq(const char *json, jsontok_t *tok, const char *s) {
        if (tok->type == JSON_STRING && (int) strlen(s) == tok->end - tok->start &&
                        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
                return 0;
        }
        return -1;
}

int dockerParseVCpus(virDomainDefPtr dom ,
                     const char *val,
                     virDomainXMLOptionPtr xmlopt){
    int vcpus ;
    char *end;
    if(virStrToLong_i(val, &end, 10, &vcpus ) < 0){
        virReportError(VIR_ERR_INTERNAL_ERROR, _("Cannot parse cpus level '%s'"),val);
        return -1;
    }
    if (virDomainDefSetVcpusMax(dom, vcpus, xmlopt) < 0)
        return -1;
    if (virDomainDefSetVcpus(dom, vcpus) < 0)
        return -1;
    return 0;
}

int dockerParseMem(virDomainDefPtr dom,
                   const char *val)
{
    unsigned long long mem;
    char *end;
    if (virStrToLong_ull(val, &end, 10, &mem) < 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("cannot parse memory level '%s'"), val);
        return -1;
    }
    virDomainDefSetMemoryTotal(dom, mem / 1024);
    dom->mem.cur_balloon = mem / 1024;

    return 0;

}

virDomainDefPtr dockerParseCommandLineString(virCapsPtr caps,
                                           virDomainXMLOptionPtr xmlopt,
                                           const char *config)
{
    int i,r;
    json_parser p;
    jsontok_t t[200];
    json_init(&p);
    r = json_parse(&p, config, strlen(config), t);
    if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
    }
    if (r < 1 || t[0].type != JSON_OBJECT) {
        printf("Object expected\n");
    }

    virDomainDefPtr def;

    if(!(def = virDomainDefNew())){
        goto error;
    }
    if(virUUIDGenerate(def->uuid) < 0){
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s" , _("failed to generate uuid"));
        goto error;
    }
    
    def->id = -1;
    def->mem.cur_balloon = 64*1024;
    VIR_DEBUG(" DEF complete ");
    virDomainDefSetMemoryTotal(def , def->mem.cur_balloon);
    def->clock.offset = VIR_DOMAIN_CLOCK_OFFSET_UTC;
    def->onReboot = VIR_DOMAIN_LIFECYCLE_RESTART;
    def->onCrash = VIR_DOMAIN_LIFECYCLE_CRASH_DESTROY;
    def->onPoweroff = VIR_DOMAIN_LIFECYCLE_DESTROY;
    def->virtType = VIR_DOMAIN_VIRT_QEMU;
    def->os.type = VIR_DOMAIN_OSTYPE_HVM;
    if (strstr("kvm", "kvm")) {
        def->virtType = VIR_DOMAIN_VIRT_KVM;
        def->features[VIR_DOMAIN_FEATURE_PAE] = VIR_TRISTATE_SWITCH_ON;
    }
    VIR_DEBUG(" DEF complete ");
    VIR_DEBUG(" DEF complete ");

        /* Loop over all keys of the root object */
    for (i = 1; i < r; i++) {
        if (jsoneq(config, &t[i], "Memory") == 0) {
            printf("- Memory: %.*s\n", t[i+1].end-t[i+1].start,
                                        config + t[i+1].start);
            char tmp[500];
            strncpy(tmp,config + t[i+1].start,t[i+1].end-t[i+1].start);
            dockerParseMem(def,tmp);
            i++;
        } else if (jsoneq(config, &t[i], "CpuShares") == 0) {
            printf("- VCPUs: %.*s\n", t[i+1].end-t[i+1].start,
                                        config + t[i+1].start);
            char tmp[500];
            strncpy(tmp,config + t[i+1].start,t[i+1].end-t[i+1].start);
            dockerParseVCpus(def,tmp,xmlopt);
            i++;
        
        }
    }
    if(caps)
        VIR_DEBUG(" Confiuration of Docker ");
    return def;
    error:
        return NULL;
}