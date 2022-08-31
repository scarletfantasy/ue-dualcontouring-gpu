/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */
#include "qef.h"
#include <stdexcept>
#include<Eigen/Eigen/Core>
#include<Eigen/Eigen/SVD>
namespace svd
{

    QefData::QefData()
    {
        this->clear();
    }

    QefData::QefData(const float ata_00, const float ata_01,
                     const float ata_02, const float ata_11, const float ata_12,
                     const float ata_22, const float atb_x, const float atb_y,
                     const float atb_z, const float btb, const float massPoint_x,
                     const float massPoint_y, const float massPoint_z,
                     const int numPoints)
    {
        this->set(ata_00, ata_01, ata_02, ata_11, ata_12, ata_22, atb_x, atb_y,
                  atb_z, btb, massPoint_x, massPoint_y, massPoint_z, numPoints);
    }

    QefData::QefData(const QefData &rhs)
    {
        this->set(rhs);
    }

	QefData& QefData::operator=(const QefData& rhs)
	{
		this->set(rhs);
		return *this;
	}

    void QefData::add(const QefData &rhs)
    {
        this->ata_00 += rhs.ata_00;
        this->ata_01 += rhs.ata_01;
        this->ata_02 += rhs.ata_02;
        this->ata_11 += rhs.ata_11;
        this->ata_12 += rhs.ata_12;
        this->ata_22 += rhs.ata_22;
        this->atb_x += rhs.atb_x;
        this->atb_y += rhs.atb_y;
        this->atb_z += rhs.atb_z;
        this->btb += rhs.btb;
        this->massPoint_x += rhs.massPoint_x;
        this->massPoint_y += rhs.massPoint_y;
        this->massPoint_z += rhs.massPoint_z;
        this->numPoints += rhs.numPoints;
    }

    void QefData::clear()
    {
        this->set(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    void QefData::set(const float mata_00, const float mata_01,
                      const float mata_02, const float mata_11, const float mata_12,
                      const float mata_22, const float matb_x, const float matb_y,
                      const float matb_z, const float mbtb, const float mmassPoint_x,
                      const float mmassPoint_y, const float mmassPoint_z,
                      const int mnumPoints)
    {
        this->ata_00 = mata_00;
        this->ata_01 = mata_01;
        this->ata_02 = mata_02;
        this->ata_11 = mata_11;
        this->ata_12 = mata_12;
        this->ata_22 = mata_22;
        this->atb_x = matb_x;
        this->atb_y = matb_y;
        this->atb_z = matb_z;
        this->btb = mbtb;
        this->massPoint_x = mmassPoint_x;
        this->massPoint_y = mmassPoint_y;
        this->massPoint_z = mmassPoint_z;
        this->numPoints = mnumPoints;
    }

    void QefData::set(const QefData &rhs)
    {
        this->set(rhs.ata_00, rhs.ata_01, rhs.ata_02, rhs.ata_11, rhs.ata_12,
                  rhs.ata_22, rhs.atb_x, rhs.atb_y, rhs.atb_z, rhs.btb,
                  rhs.massPoint_x, rhs.massPoint_y, rhs.massPoint_z,
                  rhs.numPoints);
    }
    

    QefSolver::QefSolver() : data(), ata(), atb(), massPoint(), x(),
        hasSolution(false) {}

    static void normalize(float &nx, float &ny, float &nz)
    {
        Vec3 tmpv(nx, ny, nz);
        VecUtils::normalize(tmpv);
        nx = tmpv.x;
        ny = tmpv.y;
        nz = tmpv.z;
    }

    void QefSolver::add(const float px, const float py, const float pz,
                        float nx, float ny, float nz)
    {
        this->hasSolution = false;
        normalize(nx, ny, nz);
        this->data.ata_00 += nx * nx;
        this->data.ata_01 += nx * ny;
        this->data.ata_02 += nx * nz;
        this->data.ata_11 += ny * ny;
        this->data.ata_12 += ny * nz;
        this->data.ata_22 += nz * nz;
        const float dot = nx * px + ny * py + nz * pz;
        this->data.atb_x += dot * nx;
        this->data.atb_y += dot * ny;
        this->data.atb_z += dot * nz;
        this->data.btb += dot * dot;
        this->data.massPoint_x += px;
        this->data.massPoint_y += py;
        this->data.massPoint_z += pz;
        ++this->data.numPoints;
    }

    void QefSolver::add(const Vec3 &p, const Vec3 &n)
    {
        this->add(p.x, p.y, p.z, n.x, n.y, n.z);
    }

    void QefSolver::add(const QefData &rhs)
    {
        this->hasSolution = false;
        this->data.add(rhs);
    }

	QefData QefSolver::getData()
	{
		return data;
	}

    float QefSolver::getError()
    {
        if (!this->hasSolution) {
            throw std::runtime_error("illegal state");
        }

        return this->getError(this->x);
    }

    float QefSolver::getError(const Vec3 &pos)
    {
        if (!this->hasSolution) {
            this->setAta();
            this->setAtb();
        }

        Vec3 atax;
        MatUtils::vmul_symmetric(atax, this->ata, pos);
        return VecUtils::dot(pos, atax) - 2 * VecUtils::dot(pos, this->atb)
               + this->data.btb;
    }

    void QefSolver::reset()
    {
        this->hasSolution = false;
        this->data.clear();
    }

    void QefSolver::setAta()
    {
        this->ata.setSymmetric(this->data.ata_00, this->data.ata_01,
                               this->data.ata_02, this->data.ata_11, this->data.ata_12,
                               this->data.ata_22);
    }

    void QefSolver::setAtb()
    {
        this->atb.set(this->data.atb_x, this->data.atb_y, this->data.atb_z);
    }

    float QefSolver::solve(Vec3 &outx, const float svd_tol,
                            const int svd_sweeps, const float pinv_tol)
    {
        if (this->data.numPoints == 0) {
            throw std::invalid_argument("...");
        }

        this->massPoint.set(this->data.massPoint_x, this->data.massPoint_y,
                            this->data.massPoint_z);
        VecUtils::scale(this->massPoint, 1.0f / this->data.numPoints);
        this->setAta();
        this->setAtb();
        Vec3 tmpv;
        MatUtils::vmul_symmetric(tmpv, this->ata, this->massPoint);
        VecUtils::sub(this->atb, this->atb, tmpv);
        this->x.clear();
        // Eigen::Matrix<float, 3, 3> eata;
        // eata<<this->ata.m00,this->ata.m01,this->ata.m02,this->ata.m01,this->ata.m11,this->ata.m12,this->ata.m02,this->ata.m12,this->ata.m22;
        // Eigen::Vector3f b;
        // b<<this->atb.x,this->atb.y,this->atb.z;
        // Eigen::Vector3f res=eata.llt().solve(b);
        // //Eigen::JacobiSVD<Eigen::Matrix3f,Eigen::FullPivHouseholderQRPreconditioner> esvd(eata);
        // //Eigen::Vector3f res=esvd.solve(b);
        // this->x.x=res.x();
        // this->x.y=res.y();
        // this->x.z=res.z();
        // const float result=svd::calcError(this->ata,this->x,this->atb);
        const float result = Svd::solveSymmetric(this->ata, this->atb,this->x, svd_tol, svd_sweeps, pinv_tol);

        VecUtils::addScaled(this->x, 1, this->massPoint);
        this->setAtb();
        outx.set(x);
        this->hasSolution = true;
        return result;
    }
}